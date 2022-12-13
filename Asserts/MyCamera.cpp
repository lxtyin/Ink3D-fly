/*
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

#include "MyCamera.h"

namespace Ink {

    MyViewer::MyViewer(const PerspCamera& c, float s) : camera(c), speed(s) {}

    void MyViewer::update(float dt) {

        /* get the center of window */
        auto size = Window::get_size();
        Vec2 center = Vec2(size.first, size.second) * .5;

        /* get the position of window's cursor */
        auto cursor_position = Window::get_cursor_position();
        Vec2 cursor = Vec2(cursor_position.first, cursor_position.second);
        Vec2 delta = center - cursor;

        /* update angle along Y- and Z-axis */
        axis_y += delta.x * sensitivity;
        axis_z += delta.y * sensitivity;
        if (axis_z > PI_2) axis_z = PI_2;
        if (axis_z < -PI_2) axis_z = -PI_2;

        /* move axis smoothly */
        float dty = axis_y - late_axis_y, dtz = axis_z - late_axis_z;
//        dty = dty - floor((dty + PI_2) / PI) * PI;
//        dtz = dtz - floor((dtz + PI_2) / PI) * PI;
        late_axis_y += std::clamp(dty * 0.3f, -0.3f, 0.3f);
        late_axis_z += std::clamp(dtz * 0.3f, -0.3f, 0.3f);

        /* fov back */
        float dtfov = lead_fov - camera.fov_y;
        float rate = 0.3f;
        if(dtfov < 0) rate = 0.01f;
        camera.set(camera.fov_y + dtfov * rate, camera.aspect, camera.near, camera.far);


        /* update the viewing direction of camera */
        camera.direction.x = sinf(late_axis_y) * cosf(late_axis_z);
        camera.direction.y = sinf(late_axis_z);
        camera.direction.z = cosf(late_axis_y) * cosf(late_axis_z);

        /* update the view-up vector of camera */
        camera.up.x = -sinf(late_axis_y) * sinf(late_axis_z);
        camera.up.y = cosf(late_axis_z);
        camera.up.z = -cosf(late_axis_y) * sinf(late_axis_z);

        camera.lookat(camera.position, -camera.direction, camera.up);
    }

    void MyViewer::set_fov(float fov) {
        lead_fov = fov;
    }

    void MyViewer::set_axis_y(float y){
        axis_y = y;
    }
    void MyViewer::set_axis_z(float z){
        axis_z = z;
    }

    const Camera& MyViewer::get_camera() const {
        return camera;
    }

    void MyViewer::set_position(const Vec3& p) {
        camera.position = p;
    }

    void MyViewer::set_direction(const Vec3& d) {
        Vec3 direction = -d.normalize();
        axis_z = asinf(direction.y);
        axis_y = asinf(direction.x / cosf(axis_z));
        if (std::isnan(axis_y)) axis_y = 0;
        if (cosf(axis_y) * direction.z < 0) axis_y = -axis_y + PI;
    }

}
