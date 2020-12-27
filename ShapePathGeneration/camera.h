#pragma once
#include "types.h"

class OrbitingCamera
{
	vec3 up = { 0, 1 ,0 };
	vec3 center = { 0, 0, 0 };
	float distance = 5;
	float angle_horizontal = 0;
	float angle_vertical = 0;

public:
	vec3 Position();
	mat4 View();

	void Rotate(float dh, float dv);
	void Pan(vec2 d_planar);
	void Zoom(float distance_delta);
	void Reset(vec3 center, vec3 position);
};