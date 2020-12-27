#pragma once

#include <inttypes.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;

using glm::clamp;
using glm::normalize;
using glm::dot;
using glm::cross;


using glm::perspectiveRH_ZO;
using glm::lookAtRH;


struct MemoryBuffer
{
	uint8_t *data;
	size_t size;
};

#ifdef assert
#undef assert
#endif

#define assert(cond) do {if(!(cond)) {__debugbreak();}}while(0)