//
// Created by lx_tyin on 2022/11/30.
//

#include "ParticleInstance.h"

namespace Ink {

    ParticleInstance::ParticleInstance(float interval,
                                       const std::function<void(Particle&)> &init,
                                       const std::function<void(Particle&, float dt)> &upd,
                                       const string &groupname,
                                       Renderer *r,
                                       const std::string& n) : Instance(n){
        emit_interval = interval;
        init_func = init;
        update_func = upd;
        renderer = r;
        mesh = compose_mesh = new Mesh;
        compose_mesh->groups.push_back({groupname, 0, 0});
        update_mesh();
        compose_mesh->create_normals(); //初始建立法线
    }

    void ParticleInstance::update_mesh(){
        compose_mesh->vertex.clear();
        for(Particle *p : all_particles){
            for(Vec3 v : p->vers){
                compose_mesh->vertex.push_back(p->position + v);
            }
        }
        compose_mesh->groups[0].length = compose_mesh->vertex.size();
    }

    void ParticleInstance::update(float dt){
        cumulative_time += dt;
        while(cumulative_time > emit_interval){
            cumulative_time -= emit_interval;
            Particle *nw = new Particle;
            init_func(*nw);
            all_particles.insert(nw);
        }
        std::vector<Particle*> to_del;
        for(Particle *ptr : all_particles){
            ptr->lifetime -= dt;
            if(ptr->lifetime <= 0){
                to_del.push_back(ptr);
                delete ptr;
            } else {
                update_func(*ptr, dt);
            }
        }
        for(Particle *ptr : to_del) all_particles.erase(ptr);
        update_mesh();
        renderer->unload_mesh(compose_mesh);
        renderer->load_mesh(compose_mesh);
    }

}
