//
// Created by lx_tyin on 2022/11/30.
//

#ifndef INK3D_PARTICLEINSTANCE_H
#define INK3D_PARTICLEINSTANCE_H

#include "../ink/objects/Instance.h"
#include "../ink/objects/Material.h"
#include "../ink/renderer/Renderer.h"
#include <set>

namespace Ink{
    /**
     * a simple paticle, can only modify position.
     */
    class Particle{
    public:
        std::vector<Vec3> vers;         /** < Should be a multiple of 3 */
        Vec3 position;
        float lifetime;
    };

    class ParticleInstance : public Instance {
    private:
        std::set<Particle*> all_particles;
        float cumulative_time = 0;

        Mesh *compose_mesh;
        Renderer *renderer;

        std::function<void(Particle&)> init_func;                     /**< initialize function for each particle */
        std::function<void(Particle&, float dt)> update_func;         /**< update function for each particle */

        /**
         * update all particles to mesh, so that render together.
         */
        void update_mesh();
    public:
        float emit_interval = 0.1;

        /**
         * use update_func to update all particles per frame.
         * \param dt frame deltatime
         */
        void update(float dt);

        /**
         * create a Particle system.
         *
         * \param interval emit interval
         * \param init initialize function for each particle
         * \param upd update function for each particle (No need to modify lifetime)
         * \param material only to get a name in this version, material should add to scene alone.
         * \param n instance name
         */
        explicit ParticleInstance(float interval,
                                  const std::function<void(Particle&)> &init,
                                  const std::function<void(Particle&, float dt)> &upd,
                                  const Material &material,
                                  Renderer *r,
                                  const std::string& n = "");
    };
}



#endif //INK3D_PARTICLEINSTANCE_H
