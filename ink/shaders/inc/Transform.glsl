#ifndef TRANSFORM_GLSL
#define TRANSFORM_GLSL

/* Returns true if camera is perspective. */
bool is_perspective(mat4 proj) {
	return proj[2][3] == -1.;
}

/* Returns the linearized depth with perspective camera. */
float linearize_depth_persp(float depth, float near, float far) {
	return 2. * near * far / (near + far + (depth * 2. - 1.) * (near - far));
}

/* Returns the linearized depth with orthographic camera. */
float linearize_depth_ortho(float depth, float near, float far) {
	return (near + far - (depth * 2. - 1.) * (near - far)) * .5;
}

/* Returns the linearized depth with perspective camera. */
float linearize_depth_persp(float depth, mat4 proj) {
	return proj[3][2] / (proj[2][2] + depth * 2. - 1.);
}

/* Returns the linearized depth with orthographic camera. */
float linearize_depth_ortho(float depth, mat4 proj) {
	return (proj[3][2] - depth * 2. + 1.) / proj[2][2];
}

/* Returns the linearized depth with whatever camera. */
float linearize_depth(float depth, mat4 proj) {
	float a = proj[2][3] == -1. ? 1. : 0.;
	float d = depth * 2. - 1.;
	return (proj[3][2] - d * (1. - a)) / (proj[2][2] + d * a);
}

#endif
