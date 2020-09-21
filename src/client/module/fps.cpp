#include <std_include.hpp>

#include "game_console.hpp"
#include "scheduler.hpp"

#include "game/game.hpp"
#include "game/dvars.hpp"

#include "utils/hook.hpp"
#include "utils/string.hpp"

namespace
{
	float fps_color[4] = { 0.6f, 1.0f, 0.0f, 1.0f };
	float origin_color[4] = { 1.0f, 0.67f, 0.13f, 1.0f };

	struct cg_perf_data
	{
		std::chrono::time_point<std::chrono::steady_clock> perf_start;
		std::int32_t current_ms;
		std::int32_t previous_ms;
		std::int32_t frame_ms;
		std::int32_t history[32];
		std::int32_t count;
		std::int32_t index;
		std::int32_t instant;
		std::int32_t total;
		float average;
		float variance;
		std::int32_t min;
		std::int32_t max;
	};

	cg_perf_data cg_perf = cg_perf_data();

	void perf_calc_fps(cg_perf_data* data, const std::int32_t value)
	{
		data->history[data->index % 32] = value;
		data->instant = value;
		data->min = 0x7FFFFFFF;
		data->max = 0;
		data->average = 0.0f;
		data->variance = 0.0f;
		data->total = 0;

		for (auto i = 0; i < data->count; ++i)
		{
			const std::int32_t idx = (data->index - i) % 32;

			if (idx < 0)
			{
				break;
			}

			data->total += data->history[idx];

			if (data->min > data->history[idx])
			{
				data->min = data->history[idx];
			}

			if (data->max < data->history[idx])
			{
				data->max = data->history[idx];
			}
		}

		data->average = static_cast<float>(data->total) / static_cast<float>(data->count);
		++data->index;
	}

	void perf_update()
	{
		cg_perf.count = 32;

		cg_perf.current_ms = static_cast<std::int32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - cg_perf.perf_start).count());
		cg_perf.frame_ms = cg_perf.current_ms - cg_perf.previous_ms;
		cg_perf.previous_ms = cg_perf.current_ms;

		perf_calc_fps(&cg_perf, cg_perf.frame_ms);

		utils::hook::invoke<void>(SELECT_VALUE(0x1405806E0, 0x140658E30));
	}

	void cg_draw_fps()
	{
		if (game::native::Dvar_FindVar("cg_drawFPS")->current.integer != 0 /*&& game::native::CL_IsCgameInitialized()*/)
		{
			const auto fps = static_cast<std::int32_t>(static_cast<float>(1000.0f / static_cast<float>(cg_perf.average)) + 9.313225746154785e-10);

			auto* font = game::native::R_RegisterFont("fonts/normalfont");
			if (!font) return;

			auto* const fps_string = utils::string::va("%i", fps);

			const auto scale = 1.0f;

			const auto x = (game::native::ScrPlace_GetViewPlacement()->realViewportSize[0] - 10.0f) - game::native::R_TextWidth(fps_string, 0x7FFFFFFF, font) * scale;

			const auto y = font->pixelHeight * 1.2f;

			game::native::R_AddCmdDrawText(fps_string, 0x7FFFFFFF, font, x, y, scale, scale, 0.0f, fps_color, 6);

			if (game::native::mp::g_entities && game::native::Dvar_FindVar("cg_drawFPS")->current.integer > 1 && game::native::SV_Loaded())
			{
				auto* const origin_string = utils::string::va("%f, %f, %f", game::native::mp::g_entities[0].client->ps.origin[0], game::native::mp::g_entities[0].client->ps.origin[1], game::native::mp::g_entities[0].client->ps.origin[2]);
				const auto origin_x = (game::native::ScrPlace_GetViewPlacement()->realViewportSize[0] - 10.0f) - game::native::R_TextWidth(origin_string, 0x7FFFFFFF, font) * scale;
				game::native::R_AddCmdDrawText(origin_string, 0x7FFFFFFF, font, origin_x, y + 50, scale, scale, 0.0f, origin_color, 6);
			}

		}
	}

	void cg_draw_fps_register_stub(const char* name, const char** _enum, const int value, unsigned int flags, const char* desc)
	{
		game::native::Dvar_RegisterEnum(name, _enum, value, 0x1, desc);
	}
}

class fps final : public module
{
public:
	void post_unpack() override
	{
		// fps setup
		cg_perf.perf_start = std::chrono::high_resolution_clock::now();
		utils::hook::call(SELECT_VALUE(0x140242C11, 0x1402CF457), &perf_update);

		// change cg_drawfps flags to saved
		utils::hook::call(SELECT_VALUE(0x1401F400A, 0x140272B98), &cg_draw_fps_register_stub);

		scheduler::loop(cg_draw_fps, scheduler::pipeline::renderer);
	}
};

REGISTER_MODULE(fps);