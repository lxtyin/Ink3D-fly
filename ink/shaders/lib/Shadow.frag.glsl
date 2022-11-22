#include <common>

uniform sampler2D color_map;
uniform sampler2D alpha_map;

uniform float alpha;
uniform float alpha_test;
uniform int use_color_map;
uniform int use_alpha_map;

in vec2 v_uv;

layout(location = 0) out vec4 out_color;

void main() {
	float t_alpha = alpha;
	t_alpha *= use_color_map == 1 ? texture(color_map, v_uv).w : 1.;
	t_alpha *= use_alpha_map == 1 ? texture(alpha_map, v_uv).x : 1.;
	if (t_alpha < alpha_test) discard;
}
