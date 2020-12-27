#include "intersection.h"


float FloatField::at(vec3 p)
{
	p = glm::clamp(p, 0.0f, 1.0f);
	vec3 pp = vec3(xsize, ysize, zsize) * p;
	vec3 texels[2];
	texels[0] = glm::floor(pp - 0.5f);
	texels[1] = texels[0] + 1.0f;
	vec3 a = texels[1] + 0.5f - pp;

	auto texel = [this](float fx, float fy, float fz)
	{
		int x = clamp((int)fx, 0, xsize - 1);
		int y = clamp((int)fy, 0, ysize - 1);
		int z = clamp((int)fz, 0, zsize - 1);
		float result = data[x + (z % xmap) * xtile + (z / xmap) * xtile * ytile * xmap + y * xtile * xmap];
		return result;
	};

	float s000 = texel(texels[0].x, texels[0].y, texels[0].z);
	float s001 = texel(texels[0].x, texels[0].y, texels[1].z);
	float s010 = texel(texels[0].x, texels[1].y, texels[0].z);
	float s011 = texel(texels[0].x, texels[1].y, texels[1].z);

	float s100 = texel(texels[1].x, texels[0].y, texels[0].z);
	float s101 = texel(texels[1].x, texels[0].y, texels[1].z);
	float s110 = texel(texels[1].x, texels[1].y, texels[0].z);
	float s111 = texel(texels[1].x, texels[1].y, texels[1].z);

	float result =
		s000 * a.x * a.y * a.z +
		s001 * a.x * a.y * (1 - a.z) +
		s010 * a.x * (1 - a.y) * a.z +
		s011 * a.x * (1 - a.y) * (1 - a.z) +
		s100 * (1 - a.x) * a.y * a.z +
		s101 * (1 - a.x) * a.y * (1 - a.z) +
		s110 * (1 - a.x) * (1 - a.y) * a.z +
		s111 * (1 - a.x) * (1 - a.y) * (1 - a.z)
		;

	return result;
}

OrientedPoint TraceRay(FloatField *field, vec3 ro, vec3 rd)
{
	float t = 0;

	for (int i = 0; i < 128; i++)
	{
		vec3 p = ro + rd * t;
		float d = field->at(p);

		if (d < t * 0.0001) 
		{
			break; 
		}
		if (t > 4) 
		{
			break; 
		}
		t += d;
	}
	vec3 p = ro + rd * t;
	float h = 0.0001;
	vec2 k = vec2(1, -1);
	vec3 normal = normalize(
		vec3(+1, -1, -1) * field->at(p + vec3(+1, -1, -1) * h) +
		vec3(-1, -1, +1) * field->at(p + vec3(-1, -1, +1) * h) +
		vec3(-1, +1, -1) * field->at(p + vec3(-1, +1, -1) * h) +
		vec3(+1, +1, +1) * field->at(p + vec3(+1, +1, +1) * h)
	);

	OrientedPoint result;
	result.position = p;
	result.normal = normal;
	return result;
}
