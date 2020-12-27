#include <stdio.h>
#include <SDL.h>
#include <SDL_syswm.h>

#include "types.h"
#include "utility.h"
#include "render.h"
#include "camera.h"


int main(int argc, char **argv)
{
	printf("Hello World!\n\n");
	SDL_Init(SDL_INIT_EVERYTHING);
	int window_w = 1600;
	int window_h = 900;
	float render_scale = 2.0f;
	auto window = SDL_CreateWindow("Prototype Path Generator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h, SDL_WINDOW_RESIZABLE);



	{
		SDL_SysWMinfo sys_info;
		SDL_VERSION(&sys_info.version);
		SDL_GetWindowWMInfo(window, &sys_info);

		render::Init(sys_info.info.win.window);
		render::Reset(window_w, window_h, render_scale);

	}

	OrbitingCamera camera;


	bool should_quit = false;
	while (!should_quit)
	{
		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			switch (ev.type)
			{
			case SDL_QUIT:
				should_quit = true;
				break;
			case SDL_WINDOWEVENT:
				if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				{
					window_w = ev.window.data1;
					window_h = ev.window.data2;

					render::Reset(ev.window.data1, ev.window.data2, render_scale);
				}
			default:
				break;
			}


		}

		int mouse_x;
		int mouse_y;
		uint32_t mouse_flags = SDL_GetRelativeMouseState(&mouse_x, &mouse_y);

		if (mouse_flags & SDL_BUTTON(SDL_BUTTON_LEFT))
		{

			SDL_SetRelativeMouseMode(SDL_TRUE);
			float sensitivity = 0.001f;

			float rel_x = -sensitivity * (float)mouse_x;
			float rel_y = sensitivity * (float)mouse_y;

			camera.Rotate(rel_x, rel_y);

		}
		else
		{
		SDL_SetRelativeMouseMode(SDL_FALSE);
		}


		mat4 view = camera.View();
		mat4 perspective = perspectiveRH_ZO(3.1415926f / 10.0f, (float)window_w / (float)window_h, 0.2f, 10000.0f);

		PerFrameParameters params;
		params.view = view;
		params.proj = perspective;
		params.time = SDL_GetPerformanceCounter();

		render::BeginFrame(params);
		render::UpdateSDF();

		auto plane = mat4(1);
		// render::SetArrow(plane, 3.1415926);


		render::EndFrame();
		// SDL_Delay(30);

	}

	// NOTE: bgfx shows warning on shutdown: RefCount == 3
	// this isn't our problem
	render::Shutdown();

	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}