#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "utils/hook.hpp"
#include "utils/string.hpp"
#include "game/game.hpp"

namespace slowmotion
{
	namespace
	{
		int delay = 0;
		utils::hook::detour Com_TimeScaleMsecHook;

		uint64_t Com_TimeScaleMsec(const int msec)
		{
			if (delay <= 0)
			{
				return Com_TimeScaleMsecHook.invoke<uint64_t>(msec);
			}
			else
			{
				delay -= msec;
				return 0;
			}
		}

		void ScrCmd_SetSlowMotion()
		{
			if (game::Scr_GetNumParam() < 1)
				return;

			int duration = 1000;
			float end = 1.0f;
			const float start = game::Scr_GetFloat(0);

			if (game::Scr_GetNumParam() >= 2)
				end = game::Scr_GetFloat(1u);

			if (game::Scr_GetNumParam() >= 3)
				duration = static_cast<int>(game::Scr_GetFloat(2u) * 1000.0f);

			const auto _delay = (start > end) ? 150 : 0;

			game::SV_SetConfigstring(game::CS_TIMESCALE, utils::string::va("%i %i %g %g", *game::mp::gameTime, duration, start, end));
			game::Com_SetSlowMotion(start, end, duration - _delay);

			delay = _delay;

			for (auto i = 0; i < game::Dvar_FindVar("sv_maxclients")->current.integer; i++)
			{
				auto* client = &game::mp::svs_clients[i];
				client->nextSnapshotTime = *game::mp::serverTime - 1;
			}
		}
	}

	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			if (!game::environment::is_dedi()) return;

			utils::hook::jump(0x1403B4A10, ScrCmd_SetSlowMotion);

			// Detour used here instead of call hook because Com_TimeScaleMsec is called from arxan encrypted function
			Com_TimeScaleMsecHook.create(0x140415D50, Com_TimeScaleMsec);
		}
	};
}

REGISTER_MODULE(slowmotion::module)
