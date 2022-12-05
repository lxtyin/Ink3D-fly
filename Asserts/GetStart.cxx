#include "../ink/utils/Mainloop.h"
#include "../ink/utils/Viewer.h"
#include "MyCamera.h"
#include "expend.h"
#include "ModelLoader.h"
#include "ParticleInstance.h"
#include "Remote.h"
using Ink::Vec3;

#define M_PATH "Asserts/models/"
#define P_PATH "Asserts/images/"
#define VP_WIDTH 1440
#define VP_HEIGHT 900
#define REMOTE_IP "124.223.118.118"
#define REMOTE_PORT 8888

//#define FREE_FLY //fly method.
#define USE_FORWARD_PATH 0

Ink::Scene scene;
Ink::MyViewer viewer;
Ink::Renderer renderer;

Ink::Instance *scene_obj;
Ink::Instance *plane;
Ink::Mesh *plane_mesh;
Ink::Material *plane_material;
Ink::ParticleInstance *particle_instance, *trail;
float speed = 0;

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

void conf(Settings& t) {
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

    bloom_pass = new Ink::BloomPass(VP_WIDTH, VP_HEIGHT);
    bloom_pass->init();
    bloom_pass->threshold = 0.2;
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
    renderer.load_scene(scene);
    renderer.set_viewport(Ink::Gpu::Rect(VP_WIDTH, VP_HEIGHT));
}

void load() {
    Ink::Shadow::init(4096, 4096, 4);
    Ink::Shadow::set_samples(16);

    // load parper plane, forward: z
    plane          = Ink::Instance::create();
    plane_mesh = new Ink::Mesh(Ink::Loader::load_obj(M_PATH "Plane/plane.obj")[0]);
    plane_mesh->groups[0].name = "plane_material";
    plane_material = new Ink::Material("plane_material");
    plane_material->emissive = Vec3(1, 1, 1);
    plane_mesh->create_normals();
    plane->rotation.order = Ink::EULER_YXZ;
    plane->scale = Vec3(2, 2, 2);

    plane->mesh = plane_mesh;
    scene.add(plane);
    scene.set_material(plane_material->name, plane_mesh, plane_material);

//    scene_obj = Ink::load_model(M_PATH "lakeside/lakeside_-_exterior_scene.glb", scene);
    scene_obj = Ink::load_model(M_PATH "house/low_poly_winter_scene.glb", scene);
    scene_obj->scale = Vec3(15, 15, 15);
    scene_obj->cast_shadow = false;

    // 环境光
    Ink::HemisphereLight *hemlight = new Ink::HemisphereLight();
    hemlight->ground_color = Ink::Vec3(1, 1, 1);
    hemlight->intensity = 0.2;
    scene.add_light(hemlight);

    // 平行光（阴影）
    light = new Ink::DirectionalLight();
	light->color = Ink::Vec3(0.3, 0.3, 0.3);
	light->direction = Ink::Vec3(0, -1, -0.5f);
    light->cast_shadow = true;
    light->shadow.activate();
    light->shadow.camera = Ink::OrthoCamera(-300, 300, -300, 300, 0.1, 2000);
    light->shadow.bias = 0.00005;
	scene.add_light(light);

    // 点光源
    Ink::PointLight *plight = new Ink::PointLight();
    plight->color = Ink::Vec3(0.5, 0.5, 1);
    plight->intensity = 1.5;
    plight->distance = 60;
    plight->position = Vec3(60, 40, -50);
    scene.add_light(plight);

    // 粒子
    Ink::Material *particle_mat = new Ink::Material("particle_material");
    particle_mat->emissive = Vec3(2, 2, 2);
    particle_instance = new Ink::ParticleInstance(
            0.01,
            [&](Ink::Particle &p){
                p.lifetime = 30;
                p.vers.push_back(Vec3(0, 0, 0));
                p.vers.push_back(Vec3(rand() % 10, rand() % 10, rand() % 10) / 5);
                p.vers.push_back(Vec3(rand() % 10, rand() % 10, rand() % 10) / 5);
                p.position = Vec3(rand() % 600 - 300, rand() % 20 + 130, rand() % 600 - 300);
            },
            [&](Ink::Particle &p, float dt){
                p.position -= Vec3(0, 8, 0) * dt;
            },
            particle_mat->name,
            &renderer);
    scene.set_material(particle_mat->name, particle_instance->mesh, particle_mat);
    scene.add(particle_instance);

    // 拖尾
    Ink::Material *trail_mat = new Ink::Material("trail_material");
    trail_mat->emissive = Vec3(5, 5, 10);
    trail = new Ink::ParticleInstance(
            0.03,
            [&](Ink::Particle &p){
                p.lifetime = 4;
                p.vers.push_back(Vec3(0, 0, 0));
                p.vers.push_back(Vec3(rand() % 10, rand() % 10, rand() % 10) / 10);
                p.vers.push_back(Vec3(rand() % 10, rand() % 10, rand() % 10) / 10);

                p.direction = Vec3(rand() % 10 - 5, rand() % 10 - 5, rand() % 10 - 5).normalize();

                p.position = plane->position
                        - 2 * direction_EYXZ_Z(plane->rotation)
                        + p.direction / 2;
            },
            [&](Ink::Particle &p, float dt){
                p.position += p.direction * 7 * dt;
            },
            trail_mat->name,
            &renderer);
    scene.set_material(trail_mat->name, trail->mesh, trail_mat);
    scene.add(trail);

    viewer = Ink::MyViewer(Ink::PerspCamera(75 * Ink::DEG_TO_RAD, 1.77, 0.5, 2000), 30);
    viewer.set_position(Ink::Vec3(0, 0, -2));

    renderer_load();

    remote = new Remote(REMOTE_IP, REMOTE_PORT);
}

void input_update(float dt){
    if(Ink::Window::is_down(SDLK_ESCAPE)){
        remote->logout();
        Ink::Window::close();
        exit(0);
    }
#ifdef FREE_FLY
    if(Ink::Window::is_down(SDLK_a)) horizontal_box->rotation.y += dt;
    if(Ink::Window::is_down(SDLK_d)) horizontal_box->rotation.y -= dt;
    if(Ink::Window::is_down(SDLK_w)) vertical_box->rotation.x -= dt;
    if(Ink::Window::is_down(SDLK_s)) vertical_box->rotation.x += dt;
    if(Ink::Window::is_down(SDLK_e)) plane->rotation.z += dt;
    if(Ink::Window::is_down(SDLK_q)) plane->rotation.z -= dt;
    if(Ink::Window::is_down(SDLK_SPACE)) {
        float hr = horizontal_box->rotation.y;
        float vr = vertical_box->rotation.x;
        Vec3 cur_direction = Vec3(sin(hr) * cos(vr), sin(-vr), cos(hr) * cos(vr)); //?

        horizontal_box->position += cur_direction * 5 * dt;
    }
#else
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
    }
#endif
}

void kinetic_update(float dt){

    plane->position += direction_EYXZ_Z(plane->rotation) * speed * dt;
    for(auto &[id, player] : other_plane){
        // 接不到网络update时先模拟运动
        player.instance->position += direction_EYXZ_Z(player.instance->rotation) * player.speed * dt;
    }

    viewer.set_position(plane->position + viewer.get_camera().direction * 10);
    light->position = viewer.get_camera().position - light->direction * 200;

    if(speed > 20) speed -= dt * 7;
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
            // 生成新的plane，使用新mesh 和 新material 新trail
            Player cur;
            cur.instance = Ink::Instance::create(str_format("plane_%d", st.id));

            cur.mesh = new Ink::Mesh(*plane->mesh);
            cur.mesh->groups[0].name = str_format("plane_material%d", st.id);
            renderer.load_mesh(cur.mesh);

            cur.material = new Ink::Material(str_format("plane_material%d", st.id));   //加名字以区分
            cur.material->emissive = Vec3(rand() % 30, rand() % 30, rand() % 30) / 15; //随机颜色发光

            cur.instance->mesh = cur.mesh;
            cur.instance->scale = plane->scale;

            scene.add(cur.instance);
            scene.set_material(cur.material->name, cur.mesh, cur.material);
            
            // cur.trail_mat = new Ink::Material(str_format("trail_mat%d", st.id));
            // cur.trail_mat->emissive = cur.material->emissive + Vec3(rand() % 10, rand() % 10, rand() % 10) / 10; //随机拖尾颜色偏差
            // cur.trail = new Ink::ParticleInstance(
            //     0.03,
            //     [&](Ink::Particle& p) {
            //         p.lifetime = 4;
            //         p.vers.push_back(Vec3(0, 0, 0));
            //         p.vers.push_back(Vec3(rand() % 10, rand() % 10, rand() % 10) / 10);
            //         p.vers.push_back(Vec3(rand() % 10, rand() % 10, rand() % 10) / 10);

            //         p.direction = Vec3(rand() % 10 - 5, rand() % 10 - 5, rand() % 10 - 5).normalize();

            //         p.position = cur.instance->position
            //             - 2 * direction_EYXZ_Z(cur.instance->rotation)
            //             + p.direction / 2;
            //     },
            //     [&](Ink::Particle& p, float dt) {
            //         p.position += p.direction * 7 * dt;
            //     },
            // cur.trail_mat->name,
            // &renderer);
            // scene.set_material(cur.trail_mat->name, cur.trail->mesh, cur.trail_mat);
            // scene.add(cur.trail);
            
            other_plane[st.id] = cur;
        }

        Player& cur = other_plane[st.id];
        cur.speed = st.speed;
        cur.instance->position = st.position;
        cur.instance->rotation = Ink::Euler(st.rotation, Ink::EULER_YXZ);
        cur.last_update_time = cur_time;
    }

    vector<int> to_del;
    for(auto &[id, player] : other_plane){ // 未更新的player
        if(cur_time - player.last_update_time > 1){
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
    bloom_pass->render();
    tone_map_pass->render();
    fxaa_pass->render();
#endif
}

void update(float dt) {
    input_update(dt);
    kinetic_update(dt);
    network_update(dt);

    // particle update
    trail->emit_interval = 100.0f / std::max(1.0f, speed * speed);
    trail->update(dt);
    // for(auto& [id, player] : other_plane) {
    //     player.trail->emit_interval = 100.0f / std::max(1.0f, player.speed * player.speed);
    //     player.trail->update(dt);
    // }
    particle_instance->update(dt);

    viewer.update(dt);

    renderer_update(dt);
}

void quit() {}
