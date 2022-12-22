#include "../ink/utils/Mainloop.h"
#include "../ink/utils/Viewer.h"
#include "StylizePass.h"
#include "tool.hpp"
#include "MyCamera.h"
#include "expend.h"
#include "ModelLoader.h"
#include "ParticleInstance.h"
#include "Remote.h"

using Ink::Vec3;

#define M_PATH "Asserts/models/"
#define P_PATH "Asserts/images/"

int VP_WIDTH = 1440;
int VP_HEIGHT = 900;
string REMOTE_IP = "127.0.0.1";
int REMOTE_PORT = 7777;
bool MULTI_PLAYER = false;
bool STYLIZE = false;

#define USE_FORWARD_PATH 0

Ink::Scene scene;
Ink::MyViewer viewer;
Ink::Renderer renderer;

Ink::Instance *plane;
Ink::Mesh *plane_mesh;
Ink::Material *plane_material;
Ink::ParticleInstance* snow_emitter, *trail;
float speed = 0;
float view_dis = 10;

Remote *remote;
struct Player {
	Ink::Instance *instance;
	Ink::Material* material;
	Ink::Mesh* mesh;
	Ink::ParticleInstance* trail;
	Ink::Material* trail_mat;
	float last_update_time;
	float speed;
};
std::map<int, Player> other_plane;

Ink::LightPass* light_pass = nullptr;
Ink::BloomPass* bloom_pass = nullptr;
Ink::ToneMapPass* tone_map_pass = nullptr;
Ink::FXAAPass* fxaa_pass = nullptr;
StylizePass* stylize_pass = nullptr;

Ink::DirectionalLight* light;
std::unordered_map<std::string, Ink::Image> images;

/**
 * get direction of a plane (Euler_order = YXZ, toward +Z)
 * \param e euler angle
 * \return normalized vec3 direction
 */
Vec3 direction_EYXZ_Z(Ink::Euler e){
	return Vec3(sin(e.y) * cos(e.x), -sin(e.x), cos(e.y) * cos(e.x));
}

void get_config_txt(){
	std::ifstream fin("config.txt");
	if(!fin) return;
	string name;
	while(fin >> name){
		if(name == "REMOTE_IP") fin >> REMOTE_IP;
		if(name == "REMOTE_PORT") fin >> REMOTE_PORT;
		if(name == "VP_WIDTH") fin >> VP_WIDTH;
		if(name == "VP_HEIGHT") fin >> VP_HEIGHT;
		if(name == "MULTI_PLAYER") fin >> MULTI_PLAYER;
		if(name == "STYLIZE") fin >> STYLIZE;
	}
	fin.close();
}

void conf(Settings& t) {
	get_config_txt();
	t.title = "Ink3D Example";
	t.width = VP_WIDTH;
	t.height = VP_HEIGHT;
	t.show_cursor = false;
	t.lock_cursor = true;
	t.msaa = 4;
	t.fps = 120;
	t.background_color = Ink::Vec3(1, 0.93, 0.8);
}

void renderer_load() {
#if USE_FORWARD_PATH
	renderer.set_rendering_mode(Ink::FORWARD_RENDERING);
	renderer.set_tone_mapping(Ink::ACES_FILMIC_TONE_MAP, 1);
#else

	Ink::Gpu::Texture* buffers = nullptr;
	Ink::Gpu::FrameBuffer* base_target = nullptr;

	Ink::Gpu::Texture* post_map_0 = nullptr;
	Ink::Gpu::FrameBuffer* post_target_0 = nullptr;

	Ink::Gpu::Texture* post_map_1 = nullptr;
	Ink::Gpu::FrameBuffer* post_target_1 = nullptr;

	buffers = new Ink::Gpu::Texture[5];

	buffers[0].init_2d(VP_WIDTH, VP_HEIGHT, Ink::TEXTURE_R8G8B8A8_UNORM);
	buffers[0].set_filters(Ink::TEXTURE_NEAREST, Ink::TEXTURE_NEAREST);

	buffers[1].init_2d(VP_WIDTH, VP_HEIGHT, Ink::TEXTURE_R10G10B10A2_UNORM);
	buffers[1].set_filters(Ink::TEXTURE_NEAREST, Ink::TEXTURE_NEAREST);

	buffers[2].init_2d(VP_WIDTH, VP_HEIGHT, Ink::TEXTURE_R8G8B8A8_UNORM);
	buffers[2].set_filters(Ink::TEXTURE_NEAREST, Ink::TEXTURE_NEAREST);

	buffers[3].init_2d(VP_WIDTH, VP_HEIGHT, Ink::TEXTURE_R16G16B16_SFLOAT);
	buffers[3].set_filters(Ink::TEXTURE_NEAREST, Ink::TEXTURE_NEAREST);

	buffers[4].init_2d(VP_WIDTH, VP_HEIGHT, Ink::TEXTURE_D24_UNORM);
	buffers[4].set_filters(Ink::TEXTURE_NEAREST, Ink::TEXTURE_NEAREST);

	base_target = new Ink::Gpu::FrameBuffer();
	base_target->set_attachment(buffers[0], 0);
	base_target->set_attachment(buffers[1], 1);
	base_target->set_attachment(buffers[2], 2);
	base_target->set_attachment(buffers[3], 3);
	base_target->set_depth_attachment(buffers[4]);
	base_target->draw_attachments({0, 1, 2, 3});

	post_map_0 = new Ink::Gpu::Texture();
	post_map_0->init_2d(VP_WIDTH, VP_HEIGHT, Ink::TEXTURE_R16G16B16_SFLOAT);
	post_map_0->set_filters(Ink::TEXTURE_LINEAR, Ink::TEXTURE_LINEAR);

	post_target_0 = new Ink::Gpu::FrameBuffer();
	post_target_0->set_attachment(*post_map_0, 0);

	post_map_1 = new Ink::Gpu::Texture();
	post_map_1->init_2d(VP_WIDTH, VP_HEIGHT, Ink::TEXTURE_R16G16B16_SFLOAT);
	post_map_1->set_filters(Ink::TEXTURE_LINEAR, Ink::TEXTURE_LINEAR);

	post_target_1 = new Ink::Gpu::FrameBuffer();
	post_target_1->set_attachment(*post_map_1, 0);

	renderer.set_target(base_target);

	light_pass = new Ink::LightPass();
	light_pass->init();
	light_pass->set_buffer_c(buffers + 0);
	light_pass->set_buffer_n(buffers + 1);
	light_pass->set_buffer_m(buffers + 2);
	light_pass->set_buffer_a(buffers + 3);
	light_pass->set_buffer_d(buffers + 4);
	light_pass->set_target(post_target_0);

	if(STYLIZE) {
		stylize_pass = new StylizePass();
		stylize_pass->init();
		stylize_pass->set_texture(post_map_0);
		stylize_pass->set_target(post_target_1);

		bloom_pass = new Ink::BloomPass(VP_WIDTH, VP_HEIGHT);
		bloom_pass->init();
		bloom_pass->threshold = 0.75;
		bloom_pass->radius = 0.5;
		bloom_pass->intensity = 5;
		bloom_pass->set_texture(post_map_1);
		bloom_pass->set_target(post_target_0);

		tone_map_pass = new Ink::ToneMapPass();
		tone_map_pass->init();
		tone_map_pass->mode = Ink::ACES_FILMIC_TONE_MAP;
		tone_map_pass->set_texture(post_map_0);
		tone_map_pass->set_target(post_target_1);

		fxaa_pass = new Ink::FXAAPass();
		fxaa_pass->init();
		fxaa_pass->set_texture(post_map_1);
	}
	else {
		bloom_pass = new Ink::BloomPass(VP_WIDTH, VP_HEIGHT);
		bloom_pass->init();
		bloom_pass->threshold = 0.4;
		bloom_pass->radius = 0.5;
		bloom_pass->intensity = 1;
		bloom_pass->set_texture(post_map_0);
		bloom_pass->set_target(post_target_1);

		tone_map_pass = new Ink::ToneMapPass();
		tone_map_pass->init();
		tone_map_pass->mode = Ink::ACES_FILMIC_TONE_MAP;
		tone_map_pass->set_texture(post_map_1);
		tone_map_pass->set_target(post_target_0);

		fxaa_pass = new Ink::FXAAPass();
		fxaa_pass->init();
		fxaa_pass->set_texture(post_map_0);
	}

#endif
	images["Skybox_PX"] = Ink::Loader::load_image(P_PATH "sky_px.png");
	images["Skybox_PY"] = Ink::Loader::load_image(P_PATH "sky_py.png");
	images["Skybox_PZ"] = Ink::Loader::load_image(P_PATH "sky_pz.png");
	images["Skybox_NX"] = Ink::Loader::load_image(P_PATH "sky_nx.png");
	images["Skybox_NY"] = Ink::Loader::load_image(P_PATH "sky_ny.png");
	images["Skybox_NZ"] = Ink::Loader::load_image(P_PATH "sky_nz.png");
	renderer.load_skybox(images["Skybox_PX"], images["Skybox_NX"],
						 images["Skybox_PY"], images["Skybox_NY"],
						 images["Skybox_PZ"], images["Skybox_NZ"]);

//    images["Skybox"] = Ink::Loader::load_image(P_PATH "the_sky_is_on_fire.png");
//    images["Skybox"].flip_vertical();
//    renderer.load_skybox(images["Skybox"]);

	renderer.load_scene(scene);
	renderer.set_viewport(Ink::Gpu::Rect(VP_WIDTH, VP_HEIGHT));
}

void load() {

	if(MULTI_PLAYER) remote = new Remote(REMOTE_IP, REMOTE_PORT);

	Ink::Shadow::init(4096, 4096, 4);
	Ink::Shadow::set_samples(16);

	// load parper plane, forward: z
	plane          = Ink::Instance::create();
	plane_mesh = new Ink::Mesh(Ink::Loader::load_obj(M_PATH "Plane/plane.obj")[0]);
	plane_mesh->groups[0].name = "plane_material";
	plane_material = new Ink::Material("plane_material");
	if(MULTI_PLAYER) plane_material->emissive = hash_color(remote->local_id);
	else plane_material->emissive = hash_color(0);
	plane->rotation.order = Ink::EULER_YXZ;
	plane->rotation.y = M_PI_2;
	plane->mesh = plane_mesh;
	plane->scale = Vec3(2, 2, 2);
	plane->position = Vec3(210, 254, -351);
	scene.add(plane);
	scene.set_material(plane_material->name, plane_mesh, plane_material);

	// 房子1
	Ink::Instance *scene_obj = Ink::load_model(M_PATH "house/low_poly_winter_scene.glb", scene);
	scene_obj->scale = Vec3(15, 15, 15);
	scene_obj->position = Vec3(185, 204, -301);

	// 房子2
	scene_obj = Ink::load_model(M_PATH "house/winter_country_house.glb", scene);
	scene_obj->scale = Vec3(15, 15, 15);
	scene_obj->position = Vec3(1756, 313, 204);

	// 地板
	Ink::Instance *ground = Ink::load_model(M_PATH "ground/terrain.glb", scene);
	ground->scale = Vec3(5500, 5500, 5500);

	// 环境光
	Ink::HemisphereLight *hemlight = new Ink::HemisphereLight();
	hemlight->ground_color = Ink::Vec3(1, 1, 1);
	hemlight->intensity = 0.1;
	scene.add_light(hemlight);

	// 平行光（阴影）
	light = new Ink::DirectionalLight();
	light->color = Ink::Vec3(0.7, 0.7, 0.6);
	light->direction = Ink::Vec3(-0.6, -1, 0);
	light->cast_shadow = true;
	light->shadow.activate();
	light->shadow.camera = Ink::OrthoCamera(-600, 600, -600, 600, 0.1, 2000);
	light->shadow.bias = 0.005;
	scene.add_light(light);

	// 点光源
	Ink::PointLight *plight = new Ink::PointLight();
	plight->color = Ink::Vec3(0.5, 0.5, 1);
	plight->intensity = 1.5;
	plight->distance = 100;
	plight->position = Vec3(230, 224, -351);
	scene.add_light(plight);

	// 雪花粒子
	Ink::Material *snow_mat = new Ink::Material("particle_material");
	snow_mat->emissive = Vec3(1.5, 1.5, 1.5);
	snow_mat->side = Ink::DOUBLE_SIDE;
	snow_emitter = new Ink::ParticleInstance(
			0.02,
			[&](Ink::Particle &p, Ink::Instance* ref){
				p.lifetime = 30;
				p.vers.push_back(Vec3(0, 0, 0));
				p.vers.push_back(Vec3::random() * 2);
				p.vers.push_back(Vec3::random() * 2);
				p.position = plane->position + Vec3(rand() % 800 - 400, rand() % 150, rand() % 800 - 400);
			},
			[&](Ink::Particle &p, float dt){
				p.position -= Vec3(0, 8, 0) * dt;
			},
			snow_mat->name,
				& renderer);
	scene.set_material(snow_mat->name, snow_emitter->mesh, snow_mat);
	scene.add(snow_emitter);

	// 自身拖尾（虚化
	Ink::Material *trail_mat = new Ink::Material("trail_material");
	trail_mat->emissive = plane_material->emissive + Vec3(1, 1, 0.5);
	trail_mat->side = Ink::DOUBLE_SIDE;
	trail = new Ink::ParticleInstance(
			0.05,
			[&](Ink::Particle &p, Ink::Instance *ref){
				p.lifetime = rand() % 10 * 1.0 / 10 + 4;
				p.vers.push_back(Vec3(0, 0, 0));
				p.vers.push_back(Vec3::random());
				p.vers.push_back(Vec3::random());
				p.direction = Vec3::random();

				p.position = ref->position
						- 2 * direction_EYXZ_Z(ref->rotation)
						+ p.direction / 2;
			},
			[&](Ink::Particle &p, float dt){
				p.position += p.direction * 5 * dt;
			},
			trail_mat->name,
			&renderer,
			plane);
	trail->cast_shadow = false;
	scene.set_material(trail_mat->name, trail->mesh, trail_mat);
	scene.add(trail);

	viewer = Ink::MyViewer(Ink::PerspCamera(75 * Ink::DEG_TO_RAD, 1.77, 0.5, 20000), 30);
	viewer.set_position(Ink::Vec3(0, 0, -2));

	renderer_load();
}

Player create_player(int id){
	// 生成新的plane，使用新mesh 和 新material 新trail
	Player cur;
	cur.instance = Ink::Instance::create(str_format("plane_%d", id));

	cur.mesh = new Ink::Mesh(*plane->mesh);
	cur.mesh->groups[0].name = str_format("plane_material%d", id);
	renderer.load_mesh(cur.mesh);

	cur.material = new Ink::Material(str_format("plane_material%d", id));   //加名字以区分
	cur.material->emissive = hash_color(id);

	cur.instance->mesh = cur.mesh;
	cur.instance->scale = plane->scale;

	scene.add(cur.instance);
	scene.set_material(cur.material->name, cur.mesh, cur.material);

	cur.trail_mat = new Ink::Material(str_format("trail_mat%d", id));
	cur.trail_mat->emissive = cur.material->emissive + Vec3(1, 1, 0.5); //拖尾颜色偏差
	cur.trail_mat->side = Ink::DOUBLE_SIDE;
	cur.trail = new Ink::ParticleInstance(
			0.05,
			[&](Ink::Particle& p, Ink::Instance *ref) {
				p.lifetime = rand() % 10 * 1.0 / 10 + 4;
				p.vers.push_back(Vec3(0, 0, 0));
				p.vers.push_back(Vec3::random());
				p.vers.push_back(Vec3::random());
				p.direction = Vec3::random();

				p.position = ref->position
							 - 2 * direction_EYXZ_Z(ref->rotation)
							 + p.direction / 2;
			},
			[&](Ink::Particle& p, float dt) {
				p.position += p.direction * 5 * dt;
			},
			cur.trail_mat->name,
			&renderer,
			cur.instance);
	cur.trail->cast_shadow = false;
	scene.set_material(cur.trail_mat->name, cur.trail->mesh, cur.trail_mat);
	scene.add(cur.trail);
	return cur;
}

void input_update(float dt){
	if(Ink::Window::is_down(SDLK_ESCAPE)){
		if(remote) remote->logout();
		Ink::Window::close();
		exit(0);
	}
	if(Ink::Window::is_down(SDLK_p)){
		std::cout << "Position: " << plane->position.x << ' ' << plane->position.y << ' ' << plane->position.z << '\n';
	}
	if(Ink::Window::is_down(SDLK_a)){
		plane->rotation.y += dt;
		plane->rotation.z -= dt;
	}
	if(Ink::Window::is_down(SDLK_d)){
		plane->rotation.y -= dt;
		plane->rotation.z += dt;
	}
	if(Ink::Window::is_down(SDLK_w)){
		plane->rotation.x -= dt;
	}
	if(Ink::Window::is_down(SDLK_s)){
		plane->rotation.x += dt;
	}
	if(Ink::Window::is_down(SDLK_SPACE)){
		speed = 100;
		viewer.set_fov(100 * Ink::DEG_TO_RAD);
	} else {
		viewer.set_fov(75 * Ink::DEG_TO_RAD);
	}
	if(Ink::Window::is_down(SDLK_LSHIFT)){
		speed *= 0.9;
	}
}

void kinetic_update(float dt){

	plane->position += direction_EYXZ_Z(plane->rotation) * speed * dt;
	if(speed > 30) speed -= dt * 6;
	
	for(auto& [id, player] : other_plane) {
		// 接不到网络update时先模拟运动
		player.instance->position += direction_EYXZ_Z(player.instance->rotation) * player.speed * dt;
		if(player.speed > 30) player.speed -= dt * 6;
	}

	viewer.set_position(plane->position + viewer.get_camera().direction * view_dis);
	light->position = viewer.get_camera().position - light->direction * 200;

#ifndef FREE_FLY
	plane->rotation.z *= 0.95;
	plane->rotation.x *= 0.95;
#endif
}

void network_update(float dt){
	remote->update(plane->position, Vec3(plane->rotation.x, plane->rotation.y, plane->rotation.z), speed);
	while(true){
		Status st = remote->get_status();
		if(!st.id) break;

		if(!other_plane.count(st.id)) {
			other_plane[st.id] = create_player(st.id);
		}

		Player& cur = other_plane[st.id];
		cur.speed = st.speed;
		cur.instance->position = st.position;
		cur.instance->rotation = Ink::Euler(st.rotation, Ink::EULER_YXZ);
		cur.last_update_time = cur_time;
	}

	vector<int> to_del;
	for(auto &[id, player] : other_plane){ // 一段时间未收到更新的player
		if(cur_time - player.last_update_time > 3){
			scene.remove_material(player.material->name, player.instance->mesh);
			scene.remove_material(player.trail_mat->name, player.trail->mesh);
			scene.remove(player.instance);
			scene.remove(player.trail);
			delete player.material;
			delete player.mesh;
			delete player.instance;
			delete player.trail;
			delete player.trail_mat;
			to_del.push_back(id);
		}
	}
	for(int i : to_del) other_plane.erase(i);
}

void renderer_update(float dt){
	renderer.clear();
	renderer.render_skybox(viewer.get_camera());
	renderer.update_shadow(scene, *light);
	renderer.update_scene(scene);
	renderer.render(scene, viewer.get_camera());

#if !USE_FORWARD_PATH
	light_pass->set(&scene, &viewer.get_camera());
	light_pass->render();
	if(STYLIZE) stylize_pass->render();
	bloom_pass->render();
	tone_map_pass->render();
	fxaa_pass->render();
#endif
}

void particle_update(float dt) {
	// particle update
	trail->emit_interval = speed > 90 ? 0.01 : speed > 80 ? 0.1 : 10;
	trail->update(dt);
	for(auto& [id, player] : other_plane) {
		player.trail->emit_interval = player.speed > 90 ? 0.01 : player.speed > 80 ? 0.1 : 10;
		player.trail->update(dt);
	}
	snow_emitter->update(dt);
}

void update(float dt) {

	if(MULTI_PLAYER) network_update(dt);
	input_update(dt);
	kinetic_update(dt);
	particle_update(dt);

	viewer.update(dt);

	renderer_update(dt);
}

void quit() {}
