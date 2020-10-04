#include <std_include.hpp>
#include <discord_rpc.h>
#include "loader/module_loader.hpp"
#include "scheduler.hpp"
#include "game/game.hpp"

class discord final : public module
{
public:
	void post_load() override
	{
		if (game::environment::is_dedi()) return;

		DiscordEventHandlers handlers;
		ZeroMemory(&handlers, sizeof(handlers));
		handlers.ready = ready;
		handlers.errored = errored;
		handlers.disconnected = errored;
		handlers.joinGame = nullptr;
		handlers.spectateGame = nullptr;
		handlers.joinRequest = nullptr;

		Discord_Initialize("762374436183343114", &handlers, 1, nullptr);

		scheduler::loop(Discord_RunCallbacks, scheduler::pipeline::main);
	}

	void pre_destroy() override
	{
		Discord_Shutdown();
	}

private:
	static void ready(const DiscordUser* request)
	{
		DiscordRichPresence discord_presence;
		ZeroMemory(&discord_presence, sizeof(discord_presence));

		discord_presence.details = game::environment::is_mp() ? "Multiplayer" : "Singleplayer";
		discord_presence.instance = 1;
		
#ifdef DEV_BUILD
		discord_presence.details = "Team Deathmatch";
		discord_presence.state = "Prison Break";
		discord_presence.largeImageKey = "mp_prison";
#endif
		
		Discord_UpdatePresence(&discord_presence);
	}

	static void errored(const int error_code, const char* message)
	{
		printf("Discord: (%i) %s", error_code, message);
	}
};

#ifndef DEV_BUILD
REGISTER_MODULE(discord)
#endif