/**
 * Copyright (C) 2021-2022 Hypertheory
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "../ink/math/Maths.h"
#include "../ink/camera/PerspCamera.h"
#include "../ink/window/Window.h"

namespace Ink {

    class MyViewer {
    public:
        float speed = 1;              /**< the speed of movement */
        float sensitivity = .001f;    /**< the sensitivity of mouse */

        /**
         * Creates a new MyViewer and initializes it with camera and moving speed.
         *
         * \param c camera
         * \param s moving speed
         */
        explicit MyViewer(const PerspCamera& c = PerspCamera(), float s = 1);

        /**
         * Updates the viewing camera. This function should be called every frame.
         *
         * \param dt delta time
         */
        void update(float dt);

        /**
         * set camera fov.
         * \param fov
         */
        void set_fov(float fov);

        /**
         * Update axis_y by force.
         * @param y axis_y
         */
        void set_axis_y(float y);

        /**
         * Update axis_z by force.
         * @param z axis_z
         */
        void set_axis_z(float z);

        /**
         * Returns the viewing camera. The camera will be updated after calling the
         * update function.
         */
        const Camera& get_camera() const;

        /**
         * Sets the specified position parameter of viewing camera.
         *
         * \param p position
         */
        void set_position(const Vec3& p);

        /**
         * Sets the specified direction parameter of viewing camera.
         *
         * \param d direction
         */
        void set_direction(const Vec3& d);

        PerspCamera camera;
    private:
        float axis_y = 0;
        float axis_z = 0;
        float late_axis_y = 0;
        float late_axis_z = 0;

        float lead_fov;
    };

}
