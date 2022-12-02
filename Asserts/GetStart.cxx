#include "../ink/utils/Mainloop.h"
#include "../ink/utils/Viewer.h"
#include "MyCamera.h"
#include "expend.h"
#include "ModelLoader.h"
#include "ParticleInstance.h"
using Ink::Vec3;

#define M_PATH "Asserts/models/"
#define P_PATH "Asserts/images/"
#define VP_WIDTH 1680
#define VP_HEIGHT 960

//#define FREE_FLY //fly method.
#define USE_FORWARD_PATH 0

Ink::Scene scene;
Ink::MyViewer viewer;
Ink::Renderer renderer;
Ink::Instance *scene_obj;
Ink::Instance *plane; //use 3 layer box to rotate around self-space.
Ink::ParticleInstance *particle_instance;
float speed = 0;

Ink::Mesh *plane_mesh;

Ink::LightPass* light_pass = nullptr;
Ink::BloomPass* bloom_pass = nullptr;
Ink::ToneMapPass* tone_map_pass = nullptr;
Ink::FXAAPass* fxaa_pass = nullptr;

Ink::DirectionalLight* light;

std::unordered_map<std::string, Ink::Image> images;

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
//    plane_mesh = new Ink::Mesh(Ink::Loader::load_obj(M_PATH "Plane/plane.obj")[0]);
//    Ink::Material *plane_mat = new Ink::Material(Ink::Loader::load_mtl(M_PATH "Plane/plane.mtl")[0]);
//    plane_mesh->create_normals();
    plane->rotation.order = Ink::EULER_YXZ;

    Ink::Material *plane_mat = new Ink::Material("plane_mat");
    plane_mat->emissive = Vec3(3, 3, 3);
    plane_mat->side = Ink::DOUBLE_SIDE;
    plane_mat->transparent = true;
    plane_mat->blending = true;
    plane_mat->color_map = new Ink::Image(Ink::Loader::load_image(P_PATH "y2.png"));
    plane_mat->color_map->flip_vertical();

    plane_mesh = new Ink::Mesh();
    plane_mesh->vertex.push_back(Vec3(10, 0, -10));
    plane_mesh->vertex.push_back(Vec3(-10, 0, -10));
    plane_mesh->vertex.push_back(Vec3(10, 0, 10));
    plane_mesh->vertex.push_back(Vec3(-10, 0, -10));
    plane_mesh->vertex.push_back(Vec3(-10, 0, 10));
    plane_mesh->vertex.push_back(Vec3(10, 0, 10));
    plane_mesh->uv.push_back(Ink::Vec2(0, 0));
    plane_mesh->uv.push_back(Ink::Vec2(1, 0));
    plane_mesh->uv.push_back(Ink::Vec2(0, 1));
    plane_mesh->uv.push_back(Ink::Vec2(1, 0));
    plane_mesh->uv.push_back(Ink::Vec2(1, 1));
    plane_mesh->uv.push_back(Ink::Vec2(0, 1));
    plane_mesh->create_normals();
    plane_mesh->groups.push_back({plane_mat->name, 0, 6});
    plane->scale = Vec3(0.1, 0.1, 0.1);
    plane->mesh = plane_mesh;
    scene.add(plane);
    scene.set_material(plane_mat->name, plane_mesh, plane_mat);

//    scene_obj = Ink::load_model(M_PATH "lakeside/lakeside_-_exterior_scene.glb", scene);
    scene_obj = Ink::load_model(M_PATH "house/low_poly_winter_scene.glb", scene);
    scene_obj->scale = Vec3(15, 15, 15);

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
    particle_mat->emissive = Vec3(1.5, 1.5, 1.5);
    particle_mat->side = Ink::DOUBLE_SIDE;
//    particle_mat->transparent = true;
//    particle_mat->blending = true;
//    particle_mat->color_map = new Ink::Image(Ink::Loader::load_image(P_PATH "y2.png"));
//    particle_mat->color_map->flip_vertical();
    particle_instance = new Ink::ParticleInstance(
            0.005,
            [&](Ink::Particle &p){
                p.lifetime = 40;
                Vec3 d1(Vec3(rand() % 20 - 10, 0, rand() % 20 - 10) / 2);
                Vec3 d2(Vec3(0, rand() % 10, 0) / 2);
                Vec3 d3 = d1 + d2;
                p.vers.push_back(Vec3(0, 0, 0));
                p.vers.push_back(Vec3(rand() % 20 - 10, rand() % 20, rand() % 20 - 10) / 10);
                p.vers.push_back(Vec3(rand() % 20 - 10, rand() % 20, rand() % 20 - 10) / 10);
//                p.vers.push_back(d1);/*
//                p.vers.push_back(d3);
//                p.vers.push_back(d2);*/

//                p.uv.push_back(Ink::Vec2(0, 0));
//                p.uv.push_back(Ink::Vec2(1, 0));
//                p.uv.push_back(Ink::Vec2(0, 1));
//                p.uv.push_back(Ink::Vec2(1, 0));
//                p.uv.push_back(Ink::Vec2(1, 1));
//                p.uv.push_back(Ink::Vec2(0, 1));

                p.position = Vec3(rand() % 1000 - 500, rand() % 20 + 120, rand() % 1000 - 500);
            },
            [&](Ink::Particle &p, float dt){
                p.position -= Vec3(0, 8, 0) * dt;
            },
            *particle_mat,
            &renderer);
    scene.set_material(particle_mat->name, particle_instance->mesh, particle_mat);
    scene.add(particle_instance);


    viewer = Ink::MyViewer(Ink::PerspCamera(75 * Ink::DEG_TO_RAD, 1.77, 0.5, 2000), 30);
    viewer.set_position(Ink::Vec3(0, 0, -2));

    renderer_load();
}

void input_update(float dt){
    if(Ink::Window::is_down(SDLK_ESCAPE)){
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
//    Vec3 cdir = -viewer.get_camera().direction;
//    Vec3 pdir = get_cur_direction();
//    int flip = (cdir.x * pdir.x + cdir.z * pdir.z > 0 ? 1 : -1); //flip horizontal rotate.
    int flip = 1;

    if(Ink::Window::is_down(SDLK_a)){
        plane->rotation.y += dt * flip;
        plane->rotation.z -= dt;
    }
    if(Ink::Window::is_down(SDLK_d)){
        plane->rotation.y -= dt * flip;
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
        std::cout << plane->position.x << ' '
                  << plane->position.y << ' '
                  << plane->position.z << '\n';
    }
#endif
}

void kinetic_update(float dt){

    float hr = plane->rotation.y;
    float vr = plane->rotation.x;
    Vec3 cur_direction = Vec3(sin(hr) * cos(vr), sin(-vr), cos(hr) * cos(vr)); //?

    plane->position += cur_direction * speed * dt;
    viewer.set_position(plane->position + viewer.get_camera().direction * 5);
    light->position = viewer.get_camera().position - light->direction * 200;
    particle_instance->update(dt);

    if(speed > 40) speed -= dt * 5;
#ifndef FREE_FLY
    plane->rotation.z *= 0.95;
    plane->rotation.x *= 0.95;
#endif
}

void renderer_update(float dt){
    renderer.clear();
    renderer.render_skybox(viewer.get_camera());
    renderer.update_shadow(scene, *light);
    renderer.update_scene(scene);
    renderer.render(scene, viewer.get_camera());
    renderer.render_transparent(scene, viewer.get_camera());

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
    viewer.update(dt);

    renderer_update(dt);
}

void quit() {}
