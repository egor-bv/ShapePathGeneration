#include "render.h"
#include "utility.h"
#include "intersection.h"

#include <bgfx/bgfx.h>


namespace render
{

template <typename T>
class ScopedHandle
{
	T handle;

public:

	ScopedHandle()
	{
		handle = BGFX_INVALID_HANDLE;
	}

	~ScopedHandle()
	{
		if (bgfx::isValid(handle))
		{
			bgfx::destroy(handle);
			handle = BGFX_INVALID_HANDLE;
		}
	}

	ScopedHandle &operator=(const T &free_handle)
	{
		if (handle.idx != free_handle.idx)
		{
			if (bgfx::isValid(handle))
			{
				bgfx::destroy(handle);
			}
			handle = free_handle;
		}
		return *this;
	}

	// don't generate default copy constructor
	ScopedHandle(const ScopedHandle &other) = delete;

	// implicit conversion
	operator T()
	{
		return handle;
	}
};

template <typename T>
class ScopedArray
{
	T *data = nullptr;
	size_t num_items = 0;

public:
	ScopedArray() = default;

	~ScopedArray()
	{
		delete[] data;
	}

	void ResizeClear(int count)
	{
		delete[] data;
		data = new T[count];
		num_items = count;
	}


	ScopedArray(ScopedArray &other) = delete;

	T &operator [](int index)
	{
		assert(index < num_items);
		return data[num_items];
	}

	const T &operator [](int index) const
	{
		assert(index < num_items);
		return data[num_items];
	}

	int NumItems() const
	{
		return NumItems;
	}

	T *DataPtr() const
	{
		return data;
	}
};

class PersistentResourceStorage
{
public:
	ScopedHandle<bgfx::VertexBufferHandle> mesh;

	ScopedHandle<bgfx::UniformHandle> u_time;
	ScopedHandle<bgfx::UniformHandle> u_random;
	ScopedHandle<bgfx::UniformHandle> u_tile_coord;
	ScopedHandle<bgfx::UniformHandle> u_tile_map_size;
	ScopedHandle<bgfx::UniformHandle> u_cutter_plane;
	ScopedHandle<bgfx::UniformHandle> u_pointer_inverse_transform;
	ScopedHandle<bgfx::UniformHandle> u_pointer_scale;

	ScopedHandle<bgfx::UniformHandle> s_input;
	ScopedHandle<bgfx::UniformHandle> i_sdf;

	ScopedHandle<bgfx::ShaderHandle> vs_fullscreen;
	ScopedHandle<bgfx::ShaderHandle> fs_primary;
	ScopedHandle<bgfx::ShaderHandle> fs_resolve;
	ScopedHandle<bgfx::ShaderHandle> fs_present;
	ScopedHandle<bgfx::ShaderHandle> cs_generate;

	ScopedHandle<bgfx::ProgramHandle> prog_primary;
	ScopedHandle<bgfx::ProgramHandle> prog_resolve;
	ScopedHandle<bgfx::ProgramHandle> prog_present;
	ScopedHandle<bgfx::ProgramHandle> prog_generate;

};

class FrameBufferPipeline
{
public:
	int backbuffer_width;
	int backbuffer_height;
	int render_width;
	int render_height;
	ScopedHandle<bgfx::FrameBufferHandle> fb_primary;
	ScopedHandle<bgfx::FrameBufferHandle> fb_resolved;
};


class SDFGenerator
{
public:
	int xsize, ysize, zsize;
	int xstep, ystep, zstep;
	int steps_done;
	int steps_total;
	ScopedHandle<bgfx::TextureHandle> tex_readback;
	ScopedHandle<bgfx::TextureHandle> tex_sdf;
	bool readback_started;
	uint32_t readback_ready_frame;
	bool readback_ready;
	ScopedArray<float> readback_buffer;

	bool use_tiled = true;
	int xtile, ytile;
	int xmap, ymap;

	// TODO: it's better to isolate the code for ray marching from graphics
	// i.e. pass float* and size out...
	float At(int x, int y, int z)
	{
		int index = x + y * xsize + z * xsize * ysize;
		return readback_buffer[index];
	}

	// TODO: is it even necessary? we'll see...
	float Trilinear(float x, float y, float z)
	{
		return 0;
	}
};

class ResourceContext
{
public:
	PersistentResourceStorage persistent_resources;
	FrameBufferPipeline	pipeline;
	SDFGenerator generator;
};
ResourceContext *gContext = nullptr;

enum class RenderPass : bgfx::ViewId
{
	Generate = 0,
	BlitReadback = 1,
	Primary = 10,
	Resolve,
	Present,

	Count
};

bgfx::ShaderHandle loadShader(const char *path)
{
	auto code_buffer = ReadBinaryFile(path);
	auto mem = bgfx::makeRef(code_buffer.data, code_buffer.size,
		[](void *data, void *user_data) { free(data); });
	auto result = bgfx::createShader(mem);
	return result;
}


bgfx::FrameBufferHandle createFrameBuffer(int w, int h, bool format_16f = false, bool has_depth = false)
{
	auto format = format_16f ? bgfx::TextureFormat::RGBA16F : bgfx::TextureFormat::RGBA8;
	bgfx::TextureHandle textures[2];
	textures[0] = bgfx::createTexture2D(w, h, false, 1, format,
		BGFX_TEXTURE_RT);
	if (has_depth)
	{
		textures[1] = bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::D24S8,
			BGFX_TEXTURE_RT | BGFX_TEXTURE_RT_WRITE_ONLY);
	}
	auto result = bgfx::createFrameBuffer(has_depth ? 2 : 1, textures, true);
	return result;
}

void createSDFTexture3D(SDFGenerator *gen, SDFGenerationParameters *params)
{
	int xsize = (int)floor(cbrt(params->max_volume));
	int ysize = xsize;
	int zsize = xsize;

	gen->xsize = xsize;
	gen->ysize = ysize;
	gen->zsize = zsize;

	int step = (int)floor(cbrt(params->max_volume_per_frame));
	step = (step / 4) * 4;
	step = step < 4 ? 4 : step;

	gen->xstep = step;
	gen->ystep = step;
	gen->zstep = step;

	int total_volume = xsize * ysize * zsize;
	int step_volume = step * step * step;
	gen->steps_done = 0;
	gen->steps_total = (total_volume + step_volume - 1) / (step_volume);

	gen->tex_sdf = bgfx::createTexture3D(xsize, ysize, zsize, false, bgfx::TextureFormat::R32F,
		BGFX_TEXTURE_COMPUTE_WRITE);

	gen->tex_readback = bgfx::createTexture3D(xsize, ysize, zsize, false, bgfx::TextureFormat::R32F,
		BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);
}

void createSDFTexture(SDFGenerator *gen, SDFGenerationParameters *params)
{
	int xsize = (int)floor(cbrt(params->max_volume));
	int ysize = xsize;
	int zsize = xsize;

	// NOTE: assuming zsize is a perfect square for now...

	int xmap = (int)floor(sqrt(zsize));
	int ymap = zsize / xmap;
	gen->xtile = xsize;
	gen->ytile = ysize;
	gen->xmap = xmap;
	gen->ymap = ymap;

	gen->xstep = 64;
	gen->ystep = 64;
	gen->zstep = 64;
	gen->steps_total = 1;
	gen->tex_sdf = bgfx::createTexture2D(gen->xtile * xmap, gen->ytile * ymap, false, 1, bgfx::TextureFormat::R32F,
		BGFX_TEXTURE_COMPUTE_WRITE | BGFX_SAMPLER_UVW_CLAMP | BGFX_SAMPLER_MAG_ANISOTROPIC | BGFX_SAMPLER_MIN_ANISOTROPIC);

	gen->tex_readback = bgfx::createTexture2D(gen->xtile * xmap, gen->ytile * ymap, false, 1, bgfx::TextureFormat::R32F,
		BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);

	int num_floats = gen->xtile * gen->ytile * gen->xmap * gen->ymap;
	gen->readback_buffer.ResizeClear(num_floats);
	vec4 udata = {};
	udata.x = (float)xsize;
	udata.y = (float)ysize;
	udata.z = (float)xmap;
	udata.w = (float)ymap;
	bgfx::setUniform(gContext->persistent_resources.u_tile_map_size, &udata);
}

void generateNextTile2D(bgfx::ViewId view_id, SDFGenerator *gen)
{
	auto bits_to_float = [](int t)
	{
		uint32_t ut = (uint32_t)t;
		float f = *((float *)&ut);
		return f;
	};

	vec4 udata = {};
	udata.x = bits_to_float(0);
	udata.y = bits_to_float(0);
	udata.z = bits_to_float(0);

	bgfx::setUniform(gContext->persistent_resources.u_tile_coord, &udata);
	bgfx::setImage(0, gen->tex_sdf, 0, bgfx::Access::Write, bgfx::TextureFormat::R32F);
	bgfx::dispatch(view_id, gContext->persistent_resources.prog_generate, 256 / 4, 256 / 4, 256 / 4);

	gen->steps_done = gen->steps_total;
}
void generateNextTile(bgfx::ViewId view_id, SDFGenerator *gen)
{
	int numx = gen->xstep;
	int numy = gen->ystep;
	int numz = gen->zstep;

	int xtiles = (gen->xsize + gen->xstep - 1) / gen->xstep;
	int ytiles = (gen->ysize + gen->ystep - 1) / gen->ystep;
	int ztiles = (gen->zsize + gen->zstep - 1) / gen->zstep;

	int xpart = gen->steps_done % xtiles;
	int ypart = (gen->steps_done / xtiles) % ytiles;
	int zpart = (gen->steps_done / xtiles / ytiles) % ztiles;

	auto bits_to_float = [](int t)
	{
		uint32_t ut = (uint32_t)t;
		float f = *((float *)&ut);
		return f;
	};

	vec4 udata = {};
	udata.x = bits_to_float(xpart * gen->xstep);
	udata.y = bits_to_float(ypart * gen->ystep);
	udata.z = bits_to_float(zpart * gen->zstep);

	bgfx::setUniform(gContext->persistent_resources.u_tile_coord, &udata);
	bgfx::setImage(0, gen->tex_sdf, 0, bgfx::Access::Write, bgfx::TextureFormat::R32F);
	bgfx::dispatch(view_id, gContext->persistent_resources.prog_generate, numx / 4, numy / 4, numz / 4);

	gen->steps_done++;
}





/*******************************
	Public API implementation
********************************/
// #define ROOT_PATH "C:/School/Practice1/ShapePathGeneration/ShapePathGeneration/"
#define ROOT_PATH "./"

void Init(void *native_window_handle)
{
	bgfx::Init init_params;
	init_params.type = bgfx::RendererType::Direct3D11;
	init_params.platformData.nwh = native_window_handle;

	bool success = bgfx::init(init_params);

	gContext = new ResourceContext;

	{
		auto &pr = gContext->persistent_resources;

		pr.u_time = bgfx::createUniform("u_time", bgfx::UniformType::Vec4);
		pr.u_random = bgfx::createUniform("u_random", bgfx::UniformType::Vec4);
		pr.u_tile_coord = bgfx::createUniform("u_tile_coord", bgfx::UniformType::Vec4);
		pr.u_tile_map_size = bgfx::createUniform("u_tile_map_size", bgfx::UniformType::Vec4);
		pr.u_cutter_plane = bgfx::createUniform("u_cutter_plane", bgfx::UniformType::Vec4);
		pr.u_pointer_inverse_transform = bgfx::createUniform("u_pointer_inverse_transform", bgfx::UniformType::Mat4);
		pr.u_pointer_scale = bgfx::createUniform("u_pointer_scale", bgfx::UniformType::Mat4);

		pr.s_input = bgfx::createUniform("s_input", bgfx::UniformType::Sampler);
		pr.i_sdf = bgfx::createUniform("i_sdf", bgfx::UniformType::Sampler);

		pr.vs_fullscreen = loadShader(
			ROOT_PATH "_data/shaders/dx11/vs_fullscreen_triangle.bin");
		pr.fs_primary = loadShader(
			ROOT_PATH "_data/shaders/dx11/fs_raymarch_texture_2d.bin");
		pr.fs_resolve = loadShader(
			ROOT_PATH "_data/shaders/dx11/fs_resolve.bin");
		pr.fs_present = loadShader(
			ROOT_PATH "_data/shaders/dx11/fs_present.bin");

		pr.cs_generate = loadShader(
			ROOT_PATH "_data/shaders/dx11/cs_generate_2d.bin"
		);
		pr.prog_primary = bgfx::createProgram(pr.vs_fullscreen, pr.fs_primary);
		pr.prog_resolve = bgfx::createProgram(pr.vs_fullscreen, pr.fs_resolve);
		pr.prog_present = bgfx::createProgram(pr.vs_fullscreen, pr.fs_present);
		pr.prog_generate = bgfx::createProgram(pr.cs_generate);
	}

	// TODO: init pipeline(use reset?)
}

void Reset(int backbuffer_width, int backbuffer_height, float render_scale)
{
	uint32_t reset_flags = 0
		| BGFX_RESET_VSYNC
		| BGFX_RESET_HIDPI
		;
	bgfx::reset(backbuffer_width, backbuffer_height, reset_flags);
	// TODO: update the pipeline sizes
	{
		float scale = render_scale;
		scale = scale < 0.1f ? 0.1f : scale;
		scale = scale > 2.0f ? 2.0f : scale;

		int render_width = (int)(scale * backbuffer_width);
		int render_height = (int)(scale * backbuffer_height);

		auto &pl = gContext->pipeline;

		pl.backbuffer_width = backbuffer_width;
		pl.backbuffer_height = backbuffer_height;
		pl.render_width = render_width;
		pl.render_height = render_height;
		pl.fb_primary = createFrameBuffer(render_width, render_height, true, true);
		pl.fb_resolved = createFrameBuffer(backbuffer_width, backbuffer_height, true, false);

	}
}

void Shutdown()
{
	delete gContext;
	bgfx::shutdown();
}



void BeginFrame(const PerFrameParameters &params)
{

	uint64_t common_state = 0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
		| BGFX_STATE_WRITE_Z
		;

	auto &pr = gContext->persistent_resources;
	auto &pl = gContext->pipeline;
	{

		vec3 plane_normal = normalize(vec3(1, 1, 1));
		float D = 0;
		vec4 udata_plane = vec4(plane_normal, 0);
		bgfx::setUniform(pr.u_cutter_plane, &udata_plane);

		vec3 n = plane_normal;
		vec3 q = vec3(1, 21, -71);

		vec3 u = normalize(cross(n, q));
		vec3 v = normalize(cross(n, u));

		vec3 center = vec3(0.0f);


		mat4 tf = mat4(1);
		{
			float ft = 0.1f;
			float t = (float)(params.time % 100000000) / 100000000.0f * 3.1415926 * 2;



			if(gContext->generator.readback_ready)
			{
				auto &gen = gContext->generator;
				FloatField f = {};
				f.data = gen.readback_buffer.DataPtr();
				f.xsize = 256;
				f.ysize = 256;
				f.zsize = 256;

				f.xtile = 256;
				f.ytile = 256;
				f.xmap = 16;
				f.ymap = 16;

				vec3 ro = center + u * 2.0f;
				ro = glm::rotate(ro, t, n) + vec3(0.5f);
				vec3 rd = normalize(center+vec3(0.5f) - ro);
				auto hit = TraceRay(&f, ro, rd);
				

				tf = glm::translate(tf, hit.position - 0.5f);
				mat4 orientation = glm::orientation(hit.normal, vec3(0, 1, 0));
				tf = tf * orientation;

			}

			//tf = glm::rotate(tf, t, plane_normal);
			//tf = glm::translate(tf, u * 0.5f);
			//tf = glm::rotate(tf, t, vec3(0,0,1));
		}

		mat4 inv_tf = glm::inverse(tf);
		bgfx::setUniform(pr.u_pointer_inverse_transform, &inv_tf);

		{
			if (gContext->generator.readback_ready)
			{
				auto &gen = gContext->generator;
				FloatField f = {};
				f.data = gen.readback_buffer.DataPtr();
				f.xsize = 256;
				f.ysize = 256;
				f.zsize = 256;

				f.xtile = 256;
				f.ytile = 256;
				f.xmap = 16;
				f.ymap = 16;

				vec3 ro = vec3(0.6f, 1, 0.4f);
				vec3 rd = vec3(0, -1, 0);
				auto res = TraceRay(&f, ro, rd);
				float sssss = 10;
			}
		}



		auto pass = (bgfx::ViewId)RenderPass::Primary;
		bgfx::setViewName(pass, "primary");
		bgfx::setViewFrameBuffer(pass, gContext->pipeline.fb_primary);
		bgfx::setViewClear(pass, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH);
		bgfx::setViewRect(pass, 0, 0, pl.render_width, pl.render_height);
		bgfx::setViewTransform(pass, &params.view, &params.proj);

		bgfx::setTexture(0, pr.i_sdf, gContext->generator.tex_sdf);
		bgfx::setState(common_state);
		bgfx::setVertexCount(3);
		bgfx::submit(pass, pr.prog_primary);
	}

	{
		auto pass = (bgfx::ViewId)RenderPass::Resolve;
		bgfx::setViewName(pass, "resolve");
		bgfx::setViewFrameBuffer(pass, pl.fb_resolved);
		bgfx::setViewClear(pass, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH);
		bgfx::setViewRect(pass, 0, 0, pl.backbuffer_width, pl.backbuffer_height);
		bgfx::setViewTransform(pass, &params.view, &params.proj);

		bgfx::setState(common_state);
		bgfx::setTexture(0, pr.s_input, bgfx::getTexture(pl.fb_primary));
		bgfx::setVertexCount(3);
		bgfx::submit(pass, pr.prog_resolve);
	}

	{
		auto pass = (bgfx::ViewId)RenderPass::Present;
		bgfx::setViewName(pass, "present");
		bgfx::setViewFrameBuffer(pass, BGFX_INVALID_HANDLE);
		bgfx::setViewClear(pass, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH);
		bgfx::setViewRect(pass, 0, 0, pl.backbuffer_width, pl.backbuffer_height);
		bgfx::setViewTransform(pass, &params.view, &params.proj);

		bgfx::setState(common_state);
		bgfx::setTexture(0, pr.s_input, bgfx::getTexture(pl.fb_resolved));
		bgfx::setVertexCount(3);
		bgfx::submit(pass, pr.prog_present);
	}

}

void UpdateSDF()
{
	auto &gen = gContext->generator;
	if (!isValid(gen.tex_sdf))
	{
		SDFGenerationParameters params;
		params.max_volume = 256 * 256 * 256;
		params.max_volume_per_frame = 64 * 64 * 64;

		createSDFTexture(&gen, &params);
	}

	if (gen.steps_done < gen.steps_total)
	{
		generateNextTile2D((bgfx::ViewId) RenderPass::Generate, &gen);
	}
	else if (!gen.readback_started)
	{
		if (gen.use_tiled)
		{
			bgfx::blit((bgfx::ViewId)RenderPass::BlitReadback,
				gen.tex_readback, 0, 0,
				gen.tex_sdf, 0, 0,
				gen.xtile * gen.xmap,
				gen.ytile * gen.ymap);
			gen.readback_started = true;

			gen.readback_ready_frame = bgfx::readTexture(gen.tex_readback, gen.readback_buffer.DataPtr());
		}

	}
}

void EndFrame()
{
	uint32_t frame = bgfx::frame();
	if (frame == gContext->generator.readback_ready_frame)
	{
		gContext->generator.readback_ready = true;
	}
}


}