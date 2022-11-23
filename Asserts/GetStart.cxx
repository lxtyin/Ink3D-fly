#include "../ink/utils/Mainloop.h"
#include "../ink/utils/Viewer.h"
#include "MyCamera.h"
#include "expend.h"
#include "ModelLoader.h"
using Ink::Vec3;

#define M_PATH "Asserts/models/"
const int SCREEN_W = 1680;
const int SCREEN_H = 960;
//#define FREE_FLY //fly method.

Ink::Scene scene;
Ink::MyViewer viewer;
Ink::Renderer renderer;
Ink::Instance *horizontal_box, *vertical_box, *scene_obj;
Ink::Instance *plane; //use 3 layer box to rotate around self-space.
float speed = 0;

Ink::DirectionalLight* light;

std::unordered_map<std::string, Ink::Image> images;
std::unordered_map<std::string, Ink::Material> materials;

void conf(Settings& t) {
    t.title = "Ink3D Example";
    t.width = SCREEN_W;
    t.height = SCREEN_H;
    t.show_cursor = false;
    t.lock_cursor = true;
    t.msaa = 4;
    t.fps = 60;
    t.background_color = Ink::Vec3(1, 0.93, 0.8);
}

void load() {

    Ink::Shadow::init(4096, 4096, 4);

    // load parper plane, forward: z
    horizontal_box = Ink::Instance::create();
    vertical_box   = Ink::Instance::create();
    plane          = Ink::Instance::create();
    Ink::Mesh *plane_mesh = new Ink::Mesh(Ink::Loader::load_obj(M_PATH "Plane/paper_plane.obj")[0]);
    normalize_mesh(plane_mesh);
    plane->mesh = plane_mesh;
    scene.add(horizontal_box);
    horizontal_box->add(vertical_box);
    vertical_box->add(plane);
    auto vec = Ink::Loader::load_mtl(M_PATH "Plane/material.lib");
    for(auto &x: vec){
        materials[x.name] = x;
        scene.set_material(plane_mesh, x.name, &materials[x.name]);
    }
    scene.set_material("defalut", &materials["Scene_-_Root"]);

//    scene_obj = Ink::load_model(M_PATH "lakeside/lakeside_-_exterior_scene.glb", scene);
    scene_obj = Ink::load_model(M_PATH "house/low_poly_winter_scene.glb", scene);
    scene_obj->scale = Vec3(5, 5, 5);

    // light
    light = new Ink::DirectionalLight();
	light->color = Ink::Vec3(0.6, 0.6, 0.6);
//    light->intensity = 0.4;
	light->direction = Ink::Vec3(0, -1, -1);
    light->cast_shadow = true;
    light->shadow.activate();
    light->shadow.camera = Ink::OrthoCamera(-100, 100, -100, 100, 0.1, 1000);
    light->shadow.normal_bias = 0.01;


	scene.add_light(light);
    Ink::PointLight *plight = new Ink::PointLight();
    plight->color = Ink::Vec3(1, 1, 0);
    plight->intensity = 1.5;
    plight->distance = 30;
    plight->position = Vec3(20, 10, -15);
    scene.add_light(plight);


    viewer = Ink::MyViewer(Ink::PerspCamera(75 * Ink::DEG_TO_RAD, 1.77, 0.5, 10000), 30);
    viewer.set_position(Ink::Vec3(0, 0, -2));
    
	renderer.set_rendering_mode(Ink::FORWARD_RENDERING);
//    renderer.set_texture_callback([](Ink::Gpu::Texture& t) -> void {
//        t.set_filters(Ink::TEXTURE_NEAREST, Ink::TEXTURE_NEAREST);
//    });
	
    renderer.load_scene(scene);
    renderer.set_viewport(Ink::Gpu::Rect(SCREEN_W, SCREEN_H));
}

Vec3 get_cur_direction(){
    float hr = horizontal_box->rotation.y;
    float vr = vertical_box->rotation.x;
    return Vec3(sin(hr) * cos(vr), sin(-vr), cos(hr) * cos(vr)); //?
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
    if(Ink::Window::is_down(SDLK_SPACE)) speed = 100;
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
        speed = 60;
    }
#endif
}

void kinetic_update(float dt){
    horizontal_box->position += get_cur_direction() * speed * dt;
    viewer.set_position(horizontal_box->position + viewer.get_camera().direction * 5);

    light->position = viewer.get_camera().position;

    if(speed > 20) speed *= 0.97;
#ifndef FREE_FLY
    plane->rotation.z *= 0.95;
    vertical_box->rotation.x *= 0.95;
#endif
}

void update(float dt) {
    input_update(dt);
    kinetic_update(dt);
    viewer.update(dt);
//    renderer.render_skybox(viewer.get_camera());
    renderer.update_shadow(scene, *light);
    renderer.update_scene(scene);
    renderer.render(scene, viewer.get_camera());
}

void quit() {}
