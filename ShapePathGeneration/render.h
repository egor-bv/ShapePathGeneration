#pragma once
#include "types.h"

enum class ShaderSlot
{
	RaymarchingMain,
	Resolve,
	Finalize,
	FieldFromMesh,
	Count
};

struct SDFGenerationParameters
{
	// Will create biggest possible texture not exceeding specified max_volume
	int max_volume;
	int max_volume_per_frame;
};

struct PerFrameParameters
{
	mat4 view;
	mat4 proj;
	uint64_t time;
};

namespace render
{

void Init(void *native_window_handle);
void Reset(int window_width, int window_height, float render_scale = 1.0f);
void Shutdown();

void BeginFrame(const PerFrameParameters &parameters);
void UpdateSDF();
void SetArrow(mat4 plane, float angle);
void EndFrame();

}
