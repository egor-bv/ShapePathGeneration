#include "camera.h"


const float PI = 3.1415926f;
const float TAU = 2 * PI;
const float PI_HALF = PI / 2;


float wrap_angle(float angle)
{
	float rotations = angle / TAU;
	angle -= floorf(rotations) * TAU;
	return angle;
}

float clamp_angle(float angle)
{
	const float eps = 0.01f;
	const float top = PI_HALF - eps;
	const float bottom = -PI_HALF + eps;
	angle = fmaxf(angle, bottom);
	angle = fminf(angle, top);
	return angle;
}


vec3 OrbitingCamera::Position()
{
	float pos_x = distance * sinf(angle_horizontal) * cosf(angle_vertical);
	float pos_z = distance * cosf(angle_horizontal) * cosf(angle_vertical);
	float pos_y = distance * sinf(angle_vertical);
	return { pos_x, pos_y, pos_z };
}

mat4 OrbitingCamera::View()
{
	mat4 result = lookAtRH(Position(), center, up);
	return result;
}


void OrbitingCamera::Rotate(float dh, float dv)
{
	angle_horizontal = wrap_angle(angle_horizontal + dh);
	angle_vertical = clamp_angle(angle_vertical + dv);
}
