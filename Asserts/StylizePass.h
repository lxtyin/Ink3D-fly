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

#include "../ink/postprocess/RenderPass.h"

constexpr char const* STYLIZE_VERT = R"(
#include <common>

in vec3 vertex;
in vec2 uv;

out vec2 v_uv;

void main() {
	v_uv = uv;
	gl_Position = vec4(vertex, 1.);
}
)";

constexpr char const* STYLIZE_FRAG = R"(
#include <common>

uniform sampler2D map;

in vec2 v_uv;

layout(location = 0) out vec4 out_color;

//vec3 color_1 = vec3(250, 202, 0) / 255.;
//vec3 color_2 = vec3(255, 156, 8) / 255.;
//vec3 color_3 = vec3(245, 113, 6) / 255.;
//vec3 color_4 = vec3(219, 66, 0) / 255.;
//vec3 color_5 = vec3(125, 17, 0) / 255.;

//vec3 color_1 = vec3(15, 194, 192) / 255.;
//vec3 color_2 = vec3(12, 171, 168) / 255.;
//vec3 color_3 = vec3(0, 143, 140) / 255.;
//vec3 color_4 = vec3(1, 89, 88) / 255.;
//vec3 color_5 = vec3(2, 53, 53) / 255.;

vec3 color_1 = vec3(25, 52, 65) / 255.;
vec3 color_2 = vec3(62, 96, 111) / 255.;
vec3 color_3 = vec3(145, 170, 157) / 255.;
vec3 color_4 = vec3(209, 219, 189) / 255.;
vec3 color_5 = vec3(252, 255, 245) / 255.;

void main() {
    vec3 rgb = textureLod(map, v_uv, 0).xyz;
    float s1 = length(rgb - color_1);
    float s2 = length(rgb - color_2);
    float s3 = length(rgb - color_3);
    float s4 = length(rgb - color_4);
    float s5 = length(rgb - color_5);
    float min_s = min(min(min(min(s1, s2), s3), s4), s5);
	out_color = s1 == min_s ? vec4(color_1, 0.) :
                s2 == min_s ? vec4(color_2, 0.) :
                s3 == min_s ? vec4(color_3, 0.) :
                s4 == min_s ? vec4(color_4, 0.) :
                vec4(color_5, 0.);
}
)";

class StylizePass : public Ink::RenderPass {
public:
    /**
     * Creates a new StylizePass object.
     */
    explicit StylizePass() = default;

    /**
     * Initializes the render pass and prepare the resources for rendering.
     */
    void init() override {
        stylize_shader = std::make_unique<Ink::Gpu::Shader>();
        stylize_shader->load_vert(STYLIZE_VERT);
        stylize_shader->load_frag(STYLIZE_FRAG);
        stylize_shader->compile();
    }

    /**
     * Compiles the required shaders and renders to the render target.
     */
    void render() const override {
        Ink::Gpu::Rect viewport = RenderPass::get_viewport();
        stylize_shader->use_program();
        stylize_shader->set_uniform_i("map", map->activate(0));
        RenderPass::render_to(stylize_shader.get(), target);
    }

    /**
     * Returns the 2D texture represents the input of rendering pass.
     */
    [[nodiscard]] const Ink::Gpu::Texture* get_texture() const  {
        return map;
    }

    /**
     * Sets the specified 2D texture as the input of rendering pass.
     *
     * \param t input texture
     */
    void set_texture(const Ink::Gpu::Texture* t) {
        map = t;
    }

private:
    const Ink::Gpu::Texture* map = nullptr;

    static std::unique_ptr<Ink::Gpu::Shader> stylize_shader;
};

std::unique_ptr<Ink::Gpu::Shader> StylizePass::stylize_shader;
