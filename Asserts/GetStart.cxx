#include "../ink/utils/Mainloop.h"
#include "../ink/utils/Viewer.h"
#include "MyCamera.h"
#include "expend.h"
#include "ModelLoader.h"
using Ink::Vec3;

#define M_PATH "Asserts/models/"
#define P_PATH "Asserts/images/"
#define VP_WIDTH 1680
#define VP_HEIGHT 960

#define FREE_FLY //fly method.
#define USE_FORWARD_PATH 0

Ink::Scene scene;
Ink::MyViewer viewer;
Ink::Renderer renderer;
Ink::Instance *horizontal_box, *vertical_box, *scene_obj;
Ink::Instance *plane; //use 3 layer box to rotate around self-space.
float speed = 0;

Ink::Gpu::Texture* buffers = nullptr;
Ink::Gpu::FrameBuffer* base_target = nullptr;

Ink::Gpu::Texture* post_map_0 = nullptr;
Ink::Gpu::FrameBuffer* post_target_0 = nullptr;

Ink::Gpu::Texture* post_map_1 = nullptr;
Ink::Gpu::FrameBuffer* post_target_1 = nullptr;

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

    Ink::RenderPass::set_viewport({VP_WIDTH, VP_HEIGHT});

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
    bloom_pass->threshold = 0.5;
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

    renderer.load_skybox_cubemap(images["Skybox_PX"], images["Skybox_NX"],
                                 images["Skybox_PY"], images["Skybox_NY"],
                                 images["Skybox_PZ"], images["Skybox_NZ"]);
    renderer.load_scene(scene);
    renderer.set_viewport(Ink::Gpu::Rect(VP_WIDTH, VP_HEIGHT));
}

void load() {
    Ink::Shadow::init(4096, 4096, 4);
    Ink::Shadow::set_samples(16);

    // load parper plane, forward: z
    horizontal_box = Ink::Instance::create();
    vertical_box   = Ink::Instance::create();
    plane          = Ink::Instance::create();
    Ink::Mesh *plane_mesh = new Ink::Mesh(Ink::Loader::load_obj(M_PATH "Plane/plane.obj")[0]);
    Ink::Material *plane_mat = new Ink::Material(Ink::Loader::load_mtl(M_PATH "Plane/plane.mtl")[0]);
    plane->mesh = plane_mesh;
    scene.add(horizontal_box);
    horizontal_box->add(vertical_box);
    vertical_box->add(plane);
    scene.set_material(plane_mesh, plane_mat->name, plane_mat);

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
    plight->color = Ink::Vec3(1, 1, 0);
    plight->intensity = 1.5;
    plight->distance = 60;
    plight->position = Vec3(60, 40, -50);
    scene.add_light(plight);

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
        horizontal_box->rotation.y += dt * flip;
        plane->rotation.z -= dt;
    }
    if(Ink::Window::is_down(SDLK_d)){
        horizontal_box->rotation.y -= dt * flip;
        plane->rotation.z += dt;
    }
    if(Ink::Window::is_down(SDLK_w)){
        vertical_box->rotation.x -= dt;
    }
    if(Ink::Window::is_down(SDLK_s)){
        vertical_box->rotation.x += dt;
    }
    if(Ink::Window::is_down(SDLK_SPACE)){
        speed = 100;
    }
#endif
}

void kinetic_update(float dt){

    float hr = horizontal_box->rotation.y;
    float vr = vertical_box->rotation.x;
    Vec3 cur_direction = Vec3(sin(hr) * cos(vr), sin(-vr), cos(hr) * cos(vr)); //?

    horizontal_box->position += cur_direction * speed * dt;
    viewer.set_position(horizontal_box->position + viewer.get_camera().direction * 5);
    light->position = viewer.get_camera().position - Vec3(0, -1, -0.5f) * 100;

    if(speed > 40) speed *= 0.97;
#ifndef FREE_FLY
    plane->rotation.z *= 0.95;
    vertical_box->rotation.x *= 0.95;
#endif
}

void renderer_update(float dt){
    renderer.clear();
    renderer.render_skybox(viewer.get_camera());
    renderer.update_shadow(scene, *light);
    renderer.update_scene(scene);
    renderer.render(scene, viewer.get_camera());

#if !USE_FORWARD_PATH
    light_pass->process(scene, viewer.get_camera());
    bloom_pass->process();
    tone_map_pass->process();
    fxaa_pass->process();
#endif
}

void update(float dt) {
    input_update(dt);
    kinetic_update(dt);
    viewer.update(dt);

    renderer_update(dt);
}

void quit() {}
