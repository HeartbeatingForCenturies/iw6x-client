#include <std_include.hpp>
#include "server_list.hpp"

#include "game/game.hpp"
#include "game/dvars.hpp"

#include "command.hpp"
#include "game_console.hpp"
#include "scheduler.hpp"

#include "utils/string.hpp"

const char* seh_string_ed_get_string_stub(const char* pszReference)
{
	if (!strcmp(pszReference, "PLATFORM_SYSTEM_LINK_TITLE"))
	{
		return "SERVER LIST";
	}

	if (!strcmp(pszReference, "LUA_MENU_STORE_CAPS"))
	{
		return "SERVER LIST";
	}

	if (!strcmp(pszReference, "LUA_MENU_STORE_DESC"))
	{
		return "Browse available servers.";
	}

	return game::native::SEH_StringEd_GetString(pszReference);
}

void server_list::lui_open_menu_stub(int controllerIndex, const char* menu, int a3, int a4, unsigned int a5)
{
	game::native::Cmd_ExecuteSingleCommand(0, 0, "lui_open menu_systemlink_join\n");
}

void server_list::post_unpack()
{
	if (!game::is_mp()) return;

	// change "STORE" & "LAN PARTY" to "SERVER LIST"
	utils::hook::call(0x1401F4708, &seh_string_ed_get_string_stub);

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