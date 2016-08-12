#include "game.h"
#include "game_data.h"
#include "tile_map_layer.h"
#include "texture.h"
#include "mesh.h"
#include "level.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <IL/il.h>
#include <IL/ilu.h>
#include <IL/ilut.h>

#include <memory>
#include <cassert>

namespace te {

struct Lib_init {
	Lib_init(Uint32 sdl_flags) {
		auto init = SDL_Init(sdl_flags);
		assert(init == 0);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		ilInit();
		ilClearColour(255, 255, 255, 0);
		assert(ilGetError() == IL_NO_ERROR);

		iluInit();
		ilutRenderer(ILUT_OPENGL);
	}
	~Lib_init() {
		SDL_Quit();
	}
	Lib_init(const Lib_init&) = delete;
	Lib_init& operator=(const Lib_init&) = delete;
	Lib_init(Lib_init&&) = delete;
	Lib_init& operator=(Lib_init&&) = delete;
};

class gl_context {
public:
	gl_context(SDL_Window& w)
		: context{ SDL_GL_CreateContext(&w) }
	{
		assert(context != NULL);
	}

	~gl_context()
	{
		delete_context();
	}

	gl_context(gl_context&& rhs) noexcept
		: context{ rhs.context }
	{
		rhs.context = NULL;
	}

	gl_context& operator=(gl_context&& rhs) noexcept
	{
		delete_context();
		context = rhs.context;
		rhs.context = NULL;
		return *this;
	}

	auto get() noexcept
	{
		return context;
	}

private:
	void delete_context() noexcept
	{
		if (context != NULL) {
			SDL_GL_DeleteContext(context);
		}
	}
	SDL_GLContext context;
};

} // namespace te

int main(int argc, char** argv)
{
	using namespace te;

	Lib_init sdl{SDL_INIT_VIDEO|SDL_INIT_JOYSTICK};

	std::unique_ptr<SDL_Joystick, void(*)(SDL_Joystick*)> p_joystick{nullptr, &SDL_JoystickClose};
	if (SDL_NumJoysticks > 0) {
		 p_joystick = {
			SDL_JoystickOpen(0),
			&SDL_JoystickClose
		};
	}

	auto resolution_width = 480;
	auto resolution_height = 270;
	vec2 pixel_to_world_scale{ 16.f, 16.f };
	auto window_width = resolution_width * 2;
	auto window_height = resolution_height * 2;
	std::unique_ptr<SDL_Window, void(*)(SDL_Window*)> upWindow {
		SDL_CreateWindow("Caul's Castle",
				 SDL_WINDOWPOS_CENTERED,
				 SDL_WINDOWPOS_CENTERED,
				 window_width,
				 window_height,
				 SDL_WINDOW_OPENGL|SDL_WINDOW_BORDERLESS),
		&SDL_DestroyWindow
	};
	auto pWindow = upWindow.get();
	assert(pWindow != NULL);

	gl_context context{ *pWindow };

	glViewport(0, 0, window_width, window_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, resolution_width / pixel_to_world_scale.x, resolution_height / pixel_to_world_scale.y, 0, -10000.0, 10000.0);
	assert(glGetError() == GL_NO_ERROR);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	assert(glGetError() == GL_NO_ERROR);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);

	glClearColor(0, 0, 0, 1.f);

	Game_data data{};
	data.pixel_to_world_scale = pixel_to_world_scale;

	load_level("assets/maps/arena.tmx", data);

	//data.joysticks[0] = p_joystick.get();
	//data.avatars[0] = map_id;
	//data.max_speeds[map_id] = 50;

	auto last_ticks = SDL_GetTicks();
	decltype(last_ticks) time_since_last_update = 0;
	decltype(last_ticks) time_per_frame = 1000 / 60;
	float time_per_frame_s = time_per_frame / 1000.f;

	bool run = true;
	while (run) {
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		auto curr_ticks = SDL_GetTicks();
		auto dt = curr_ticks - last_ticks;
		time_since_last_update += dt;

		while (time_since_last_update > time_per_frame) {
			time_since_last_update -= time_per_frame;
			SDL_Event evt;
			while (SDL_PollEvent(&evt)) {
				if (evt.type == SDL_QUIT || (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_ESCAPE)) {
					run = false;
				}
				input_game(data, evt);
			}
			step_game(data, time_per_frame_s);
		}

		draw_game(data);
		SDL_GL_SwapWindow(pWindow);

		last_ticks = curr_ticks;
	}

	return 0;
}
