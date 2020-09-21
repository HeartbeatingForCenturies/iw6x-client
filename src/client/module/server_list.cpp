#include <std_include.hpp>
#include "server_list.hpp"

#include "game/game.hpp"
#include "game/dvars.hpp"

#include "command.hpp"
#include "game_console.hpp"
#include "localized_strings.hpp"
#include "scheduler.hpp"

#include "utils/string.hpp"

void lui_open_menu_stub(int controllerIndex, const char* menu, int a3, int a4, unsigned int a5)
{
	game::native::Cmd_ExecuteSingleCommand(0, 0, "lui_open menu_systemlink_join\n");
}

void server_list::post_unpack()
{
	if (!game::is_mp()) return;

	localized_strings::override("PLATFORM_SYSTEM_LINK_TITLE", "SERVER LIST");
	localized_strings::override("LUA_MENU_STORE_CAPS", "SERVER LIST");
	localized_strings::override("LUA_MENU_STORE_DESC", "Browse available servers.");

	// hook LUI_OpenMenu to show server list
	utils::hook::call(0x1404FE840, &lui_open_menu_stub);
	
	// add lui_open command
	command::add("lui_open", [](command::params params)
	{
		if (params.size() <= 1)
		{
			game_console::print(7, "usage: lui_open <name>\n");
			return;
		}

		game::native::LUI_OpenMenu(0, params[1], 1, 0, 0);
	});
}

REGISTER_MODULE(server_list);