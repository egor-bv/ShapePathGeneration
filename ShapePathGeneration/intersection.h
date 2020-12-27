#pragma once
#include "types.h"

struct OrientedPoint
{
	vec3 position;
	vec3 normal;
};

struct FloatField
{
	float *data;

	int xsize;
	int ysize;
	int zsize;

	int xtile;
	int ytile;
	int xmap;
	int ymap;

	float at(vec3 p);
};


OrientedPoint TraceRay(FloatField *field, vec3 ro, vec3 rd);