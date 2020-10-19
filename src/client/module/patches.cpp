#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "command.hpp"
#include "game_console.hpp"
#include "game/game.hpp"
#include "game/dvars.hpp"
#include "filesystem.hpp"

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

		bool cmd_exec_patch()
		{
			command::params exec_params;
			if (exec_params.size() == 2)
			{
				std::string file_name = exec_params.get(1);
				if (file_name.find(".cfg") == std::string::npos)
					file_name.append(".cfg");

				const auto file = filesystem::file(file_name);
				if (file.exists())
				{
					game::Cbuf_ExecuteBufferInternal(0, 0, file.get_buffer().data(), game::Cmd_ExecuteSingleCommand);
					return true;
				}				
			}

			return false;
		}

		auto cmd_exec_stub_mp = utils::hook::assemble([](utils::hook::assembler& a)
		{
			const auto success = a.newLabel();

			a.pushad64();
			a.call(cmd_exec_patch);
			a.test(al, al);
			a.popad64();

			a.jz(success);
			a.mov(edx, 0x18000);
			a.jmp(0x1403F7530);

			a.bind(success);
			a.jmp(0x1403F7574);
		});

		auto cmd_exec_stub_sp = utils::hook::assemble([](utils::hook::assembler& a)
		{
			const auto success = a.newLabel();

			a.pushad64();
			a.call(cmd_exec_patch);
			a.test(al, al);
			a.popad64();

			a.jz(success);
			a.mov(edx, 0x18000);
			a.jmp(0x1403B39C0);

			a.bind(success);
			a.jmp(0x1403B3A04);
		});
	}

	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			command::add("quit", []()
			{
				game::Com_Quit();
			});

			command::add("crash", []()
			{
				*reinterpret_cast<int*>(1) = 0;
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


			if (!game::environment::is_sp())
			{
				//increased max limit for sv_network_fps, the lower limit is the default one. Original range is from 20 to 200 times a second.
				game::Dvar_RegisterInt("sv_network_fps", 1000, 20, 1000, 0, "Number of times per second the server checks for net messages");
			}

			// Register cg_fovscale with new params
			utils::hook::call(SELECT_VALUE(0x140317079, 0x140272777), register_fovscale_stub);

			command::add("getDvarValue", [](command::params& params)
			{
				if (params.size() != 2)
				{
					return;
				}
				
				auto dvar = game::Dvar_FindVar(params.get(1));
				if (dvar)
				{
					auto value = game::Dvar_ValueToString(dvar, dvar->current);
					game_console::print(7, "%s current value: %s", dvar->name, value);
				}
			});

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

			// Allow executing custom cfg files with the "exec" command
			utils::hook::jump(SELECT_VALUE(0x1403B39BB, 0x1403F752B), SELECT_VALUE(0x1403B3A12, 0x1403F7582)); //Use a relative jump to empty memory first
			utils::hook::jump(SELECT_VALUE(0x1403B3A12, 0x1403F7582), SELECT_VALUE(cmd_exec_stub_sp, cmd_exec_stub_mp), true); //Use empty memory to go to our stub first (can't do close jump, so need space for 12 bytes)

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

			// block changing name in-game
			utils::hook::set<uint8_t>(0x140470300, 0xC3);

			// Unlock all patches/cardtitles and exclusive items/camos
			utils::hook::set(0x140402B10, 0xC301B0); // LiveStorage_IsItemUnlockedFromTable_LocalClient
			utils::hook::set(0x140402360, 0xC301B0); // LiveStorage_IsItemUnlockedFromTable
			utils::hook::set(0x1404A94E0, 0xC301B0); // GetIsCardTitleUnlocked

			// Enable DLC items, extra loadouts and map selection in extinction
			dvar_register_int_hook.create(0x1404EE270, &dvar_register_int);
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
