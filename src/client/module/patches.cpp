#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "command.hpp"
#include "game_console.hpp"
#include "game/game.hpp"
#include "game/dvars.hpp"

#include "utils/hook.hpp"
#include "utils/nt.hpp"

namespace patches
{
	namespace
	{
		utils::hook::detour live_get_local_client_name_hook;

		const char* live_get_local_client_name()
		{
			return game::Dvar_FindVar("name")->current.string;
		}

		utils::hook::detour dvar_register_int_hook;

		game::dvar_t* dvar_register_int(const char* dvarName, int value, int min, int max, unsigned int flags,
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

			return dvar_register_int_hook.invoke<game::dvar_t*>(dvarName, value, min, max, flags, description);
		}

		game::dvar_t* register_fovscale_stub(const char* name, float /*value*/, float /*min*/, float /*max*/,
		                                     unsigned int /*flags*/,
		                                     const char* desc)
		{
			// changed max value from 2.0f -> 5.0f and min value from 0.5f -> 0.1f
			return game::Dvar_RegisterFloat(name, 1.0f, 0.1f, 5.0f, 0x1, desc);
		}
	}

	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			command::add("quit", []()
			{
				utils::hook::invoke<void>(SELECT_VALUE(0x1403BDDD0, 0x140414920));
			});

			command::add("quit_hard", []()
			{
				utils::nt::raise_hard_exception();
			});

			// Keep these at 1 so they cannot be used 
			// For colorMap and lightMap : 1 = "Unchanged"
			game::Dvar_RegisterInt("r_fog", 1, 1, 1, 0, "Shows the maps fog");
			game::Dvar_RegisterInt("fx_draw", 1, 1, 1, 0, "Toggles drawing of effects after processing");
			game::Dvar_RegisterInt("fx_enable", 1, 1, 1, 0, "Toggles all effects processing");
			game::Dvar_RegisterInt("r_colorMap", 1, 1, 1, 0, "Replace all color maps with pure black or pure white");
			game::Dvar_RegisterInt("r_lightMap", 1, 1, 1, 0, "Replace all lightmaps with pure black or pure white");

			// Keeping it so it cant be used for uav cheats for people
			game::Dvar_RegisterInt("bg_compassShowEnemies", 0, 0, 0, 0x8C,
			                       "Whether enemies are visible on the compass at all times");

			// set it to 3 to display both voice dlc announcers did only show 1
			game::Dvar_RegisterInt("igs_announcer", 3, 3, 3, 0x0,
			                       "Show Announcer Packs. (Bitfield representing which announcer paks to show)");

			// changed max value from 85 -> 1000
			if (game::environment::is_dedi())
			{
				game::Dvar_RegisterInt("com_maxfps", 0, 0, 0, 0, "Cap frames per second");
			}
			else
			{
				game::Dvar_RegisterInt("com_maxfps", 85, 0, 1000, 0x1, "Cap frames per second");
			}

			// changed max value from 80.0f -> 120.f and min value from 65.0f -> 1.0f
			game::Dvar_RegisterFloat("cg_fov", 65.0f, 1.0f, 120.0f, 0x1, "The field of view angle in degrees");

			// maybe _x can stay usable within a reasonable range? it can make scoped weapons DRASTICALLY better on high FOVs
			game::Dvar_RegisterFloat("cg_gun_x", 0.0f, -1.0f, 2.0f, 0x1, "Forward position of the viewmodel");
			game::Dvar_RegisterInt("cg_gun_y", 0, 0, 0, 0, "Right position of the viewmodel");
			game::Dvar_RegisterInt("cg_gun_z", 0, 0, 0, 0, "Up position of the viewmodel");

			command::add("dvarDump", []()
			{
				game_console::print(
					7, "================================ DVAR DUMP ========================================\n");
				int i;
				for (i = 0; i < *game::dvarCount; i++)
				{
					if (game::sortedDvars[i] && game::sortedDvars[i]->name)
					{
						game_console::print(7, "%s\n", game::sortedDvars[i]->name);
					}
				}
				game_console::print(7, "\n%i dvar indexes\n", i);
				game_console::print(
					7, "================================ END DVAR DUMP ====================================\n");
			});

			command::add("commandDump", []()
			{
				game_console::print(
					7, "================================ COMMAND DUMP =====================================\n");
				game::cmd_function_s* cmd = (*game::cmd_functions);
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
				game_console::print(
					7, "================================ END COMMAND DUMP =================================\n");
			});

			if (game::environment::is_sp())
			{
				patch_sp();
			}
			else
			{
				patch_mp();
			}
		}

		void patch_mp() const
		{
			// Use name dvar and add "saved" flags to it
			utils::hook::set<uint8_t>(0x1402C836D, 0x01);
			live_get_local_client_name_hook.create(0x1404FDAA0, &live_get_local_client_name);

			// Unlock all patches/cardtitles and exclusive items/camos
			utils::hook::set(0x140402B10, 0xC301B0); // LiveStorage_IsItemUnlockedFromTable_LocalClient
			utils::hook::set(0x140402360, 0xC301B0); // LiveStorage_IsItemUnlockedFromTable
			utils::hook::set(0x1404A94E0, 0xC301B0); // GetIsCardTitleUnlocked

			// Enable DLC items, extra loadouts and map selection in extinction
			dvar_register_int_hook.create(0x1404EE270, &dvar_register_int);

			// Register cg_fovscale with new params
			utils::hook::call(0x140272777, register_fovscale_stub);
		}

		void patch_sp() const
		{
			// SP doesn't initialize WSA
			WSADATA wsa_data;
			WSAStartup(MAKEWORD(2, 2), &wsa_data);
		}
	};
}

REGISTER_MODULE(patches::module)
