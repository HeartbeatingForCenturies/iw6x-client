#include <std_include.hpp>
#include "patches.hpp"

#include "command.hpp"
#include "game_console.hpp"
#include "game/game.hpp"

#include "utils/string.hpp"
#include "utils/hook.hpp"
#include "utils/nt.hpp"

utils::hook::detour live_get_local_client_name_hook;

const char* live_get_local_client_name()
{
	return game::native::Dvar_FindVar("name")->current.string;
}

utils::hook::detour dvar_register_int_hook;

game::native::dvar_t* dvar_register_int(const char* dvarName, int value, int min, int max, unsigned int flags,
                                        const char* description)
{
	// enable map selection in extinction
	if (!strcmp(dvarName, "extinction_map_selection_enabled"))
	{
		value = true;
	}

		// enable extra loadouts
	else if (!strcmp(dvarName, "extendedLoadoutsEnable"))
	{
		value = true;
	}

		// show all in-game store items
	else if (strstr(dvarName, "igs_"))
	{
		value = true;
	}

	return dvar_register_int_hook.invoke<game::native::dvar_t*>(dvarName, value, min, max, flags, description);
}

void patches::post_unpack()
{
	//Patch r_znear "does not like being forced to be set at its default value fucks with rendering"
	//game::native::Dvar_RegisterInt("r_znear", 4, 0, 4, 0x1, "near Z clip plane distance"); // keeping it at for as it can be used as wall hacks if set more

	// Patch bg_compassshowenemies
	game::native::Dvar_RegisterInt("bg_compassshowenemies", 0, 0, 0, 0x1, "always on uav");
	// Keeping it so it cant be used for uav cheats for people

	// igs_announcer
	game::native::Dvar_RegisterInt("igs_announcer", 3, 3, 3, 0x1, "dlc voice announcers");
	// set it to 3 to display both voice dlc announcers did only show 1

	// patch com_maxfps
	game::native::Dvar_RegisterInt("com_maxfps", 85, 0, 1000, 0x1, "Cap frames per second");
	// changed max value from 85 -> 1000

	// patch cg_fov
	game::native::Dvar_RegisterFloat("cg_fov", 65.0f, 65.0f, 120.0f, 0x1, "The field of view angle in degrees");
	// changed max value from 80.0f -> 120.f

	// patch r_vsync
	game::native::Dvar_RegisterBool("r_vsync", false, 0x1,
	                                "Enable v-sync before drawing the next frame to avoid 'tearing' artifacts.");
	// changed default value from true -> false. There are some fps issues with vsync at least on 144hz monitors.

	// add dvarDump command
	command::add("dvarDump", [](command::params&)
	{
		game_console::print(7, "================================ DVAR DUMP ========================================\n");
		int i;
		for (i = 0; i < *game::native::dvarCount; i++)
		{
			if (game::native::sortedDvars[i] && game::native::sortedDvars[i]->name)
			{
				game_console::print(7, "%s\n", game::native::sortedDvars[i]->name);
			}
		}
		game_console::print(7, "\n%i dvar indexes\n", i);
		game_console::print(7, "================================ END DVAR DUMP ====================================\n");
	});

	// add commandDump command
	command::add("commandDump", [](command::params&)
	{
		game_console::print(7, "================================ COMMAND DUMP =====================================\n");
		game::native::cmd_function_s* cmd = (*game::native::cmd_functions);
		int i = 0;
		while (cmd)
		{
			if (cmd->name)
			{
				game_console::print(7, "%s\n", cmd->name);
				i++;
			}
			cmd = cmd->next;
		}
		game_console::print(7, "\n%i command indexes\n", i);
		game_console::print(7, "================================ END COMMAND DUMP =================================\n");
	});

	if (game::is_mp())
		mp();
	else if (game::is_sp())
		sp();
}

void patches::mp()
{
	// add quit command
	command::add("quit", [](command::params&)
	{
		utils::hook::invoke<void>(0x140414920);
	});

	// add quit_hard command
	command::add("quit_hard", [](command::params&)
	{
		utils::nt::raise_hard_exception();
	});

	// Use name dvar and add "saved" flags to it
	utils::hook::set<uint8_t>(0x1402C836D, 0x01);
	live_get_local_client_name_hook.create(0x1404FDAA0, &live_get_local_client_name);

	// Unlock all patches/cardtitles and exclusive items/camos
	utils::hook::set(0x140402B10, 0xC301B0); // LiveStorage_IsItemUnlockedFromTable_LocalClient
	utils::hook::set(0x140402360, 0xC301B0); // LiveStorage_IsItemUnlockedFromTable
	utils::hook::set(0x1404A94E0, 0xC301B0); // GetIsCardTitleUnlocked

	// Enable DLC items, extra loadouts and map selection in extinction
	dvar_register_int_hook.create(0x1404EE270, &dvar_register_int);
}

void patches::sp()
{
	// SP doesn't initialize WSA
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
}

REGISTER_MODULE(patches);
