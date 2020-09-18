#include <std_include.hpp>
#include "patches.hpp"

#include "game_console.hpp"
#include "game/game.hpp"

#include "utils/string.hpp"
#include "utils/hook.hpp"

utils::hook::detour live_get_local_client_name_hook;
const char* live_get_local_client_name()
{
	return game::native::Dvar_FindVar("name")->current.string;
}

utils::hook::detour dvar_register_int_hook;
game::native::dvar_t* dvar_register_int(const char* dvarName, int value, int min, int max, unsigned int flags, const char* description)
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
	if (!game::is_mp()) return;

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

REGISTER_MODULE(patches);
