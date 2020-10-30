#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "scheduler.hpp"
#include "game/game.hpp"

#include "utils/string.hpp"

#include <discord_rpc.h>

namespace discord
{
	namespace
	{
		DiscordRichPresence discord_presence;

		void update_discord()
		{
			Discord_RunCallbacks();

			if (!game::CL_IsCgameInitialized())
			{
				discord_presence.details = game::environment::is_sp() ? "Singleplayer" : "Multiplayer";
				discord_presence.state = "Main Menu";

				discord_presence.partySize = 0;
				discord_presence.partyMax = 0;

				discord_presence.startTimestamp = 0;

				discord_presence.largeImageKey = game::environment::is_sp() ? "menu_singleplayer" : "menu_multiplayer";
			}
			else
			{
				if (game::environment::is_sp()) return;

				const auto* gametype = game::UI_LocalizeGametype(game::Dvar_FindVar("ui_gametype")->current.string);
				const auto* map = game::UI_LocalizeMapname(game::Dvar_FindVar("ui_mapname")->current.string);

				discord_presence.details = utils::string::va("%s on %s", gametype, map);

				auto* const host_name = reinterpret_cast<char*>(0x14187EBC4);
				utils::string::strip(host_name, host_name, static_cast<int>(strlen(host_name)) + 1);
				
				discord_presence.state = host_name;

				discord_presence.partySize = game::mp::cgArray->snap != nullptr
					                             ? game::mp::cgArray->snap->numClients
					                             : 1;
				discord_presence.partyMax = game::Dvar_FindVar("sv_maxclients")->current.integer;

				if (!discord_presence.startTimestamp)
				{
					discord_presence.startTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
						std::chrono::system_clock::now().time_since_epoch()).count();
				}

				discord_presence.largeImageKey = game::Dvar_FindVar("ui_mapname")->current.string;
			}

			Discord_UpdatePresence(&discord_presence);
		}
	}

	class module final : public module_interface
	{
	public:
		void post_load() override
		{
			if (game::environment::is_dedi())
			{
				return;
			}

			DiscordEventHandlers handlers;
			ZeroMemory(&handlers, sizeof(handlers));
			handlers.ready = ready;
			handlers.errored = errored;
			handlers.disconnected = errored;
			handlers.joinGame = nullptr;
			handlers.spectateGame = nullptr;
			handlers.joinRequest = nullptr;

			Discord_Initialize("762374436183343114", &handlers, 1, nullptr);

			scheduler::loop(update_discord, scheduler::pipeline::async, 20s);

			initialized_ = true;
		}

		void pre_destroy() override
		{
			if (!initialized_ || game::environment::is_dedi())
			{
				return;
			}

			Discord_Shutdown();
		}

	private:
		bool initialized_ = false;

		static void ready(const DiscordUser* request)
		{
			ZeroMemory(&discord_presence, sizeof(discord_presence));

			discord_presence.instance = 1;

			Discord_UpdatePresence(&discord_presence);
		}

		static void errored(const int error_code, const char* message)
		{
			printf("Discord: (%i) %s", error_code, message);
		}
	};
}

#ifndef DEV_BUILD
REGISTER_MODULE(discord::module)
#endif
