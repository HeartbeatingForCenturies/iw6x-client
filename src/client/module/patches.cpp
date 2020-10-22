#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "command.hpp"
#include "game_console.hpp"
#include "game/game.hpp"
#include "game/dvars.hpp"
#include "filesystem.hpp"
#include "scheduler.hpp"

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

		game::dvar_t* register_cg_gun_dvars(const char* name, float /*value*/, float /*min*/, float /*max*/,
											unsigned int /*flags*/, const char* desc)
		{
			if (name == "cg_gun_x"s)
			{
				return game::Dvar_RegisterFloat(name, 0.0f, -1.0f, 2.0f, game::DvarFlags::DVAR_FLAG_SAVED, desc);
			}
			else
			{
				return game::Dvar_RegisterFloat(name, 0.0f, 0.0f, 0.0f, 0, desc);
			}
    }
    
		game::dvar_t* register_network_fps_stub(const char* name, int, int, int, unsigned int flags,
		                                        const char* desc)
		{
			return game::Dvar_RegisterInt(name, 1000, 20, 1000, flags, desc);
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

		int dvar_command_patch() // game makes this return an int and compares with eax instead of al -_-
		{
			command::params args{};

			if (args.size() <= 0)
				return 0;

			auto* dvar = game::Dvar_FindVar(args.get(0));
			if (dvar)
			{
				if (args.size() == 1)
				{
					const auto current = game::Dvar_ValueToString(dvar, dvar->current);
					const auto reset = game::Dvar_ValueToString(dvar, dvar->reset);
					game_console::print(7, "\"%s\" is: \"%s^7\" default: \"%s^7\"", dvar->name, current, reset);
					game_console::print(7, "   %s\n", dvars::dvar_get_domain(dvar->type, dvar->domain).data());
				}
				else
				{
					char command[0x1000] = { 0 };
					game::Dvar_GetCombinedString(command, 1);
					game::Dvar_SetCommand(args.get(0), command);
				}

				return 1;
			}

			return 0;
		}
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
			if (!game::environment::is_dedi())
			{
				game::Dvar_RegisterInt("com_maxfps", 85, 0, 1000, 0x1, "Cap frames per second");
			}

			if (!game::environment::is_sp())
			{
				//increased max limit for sv_network_fps, the lower limit is the default one. Original range is from 20 to 200 times a second.
				utils::hook::call(0x140476F4F, register_network_fps_stub);
			}

			// register cg_gun_ dvars with new values and flags
			// maybe _x can stay usable within a reasonable range? it can make scoped weapons DRASTICALLY better on high FOVs
			utils::hook::call(SELECT_VALUE(0x140228DDE, 0x1402AB04C), register_cg_gun_dvars);
			utils::hook::call(SELECT_VALUE(0x140228E0E, 0x1402AB07C), register_cg_gun_dvars);
			utils::hook::call(SELECT_VALUE(0x140228E3E, 0x1402AB0AC), register_cg_gun_dvars);

			// Register cg_fovscale with new params
			utils::hook::call(SELECT_VALUE(0x140317079, 0x140272777), register_fovscale_stub);

			// Patch Dvar_Command to print out values how CoD4 does it
			utils::hook::jump(SELECT_VALUE(0x1403BFCB0, 0x140416A60), dvar_command_patch);

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
			utils::hook::jump(SELECT_VALUE(0x1403B39BB, 0x1403F752B), SELECT_VALUE(0x1403B3A12, 0x1403F7582));
			//Use a relative jump to empty memory first
			utils::hook::jump(SELECT_VALUE(0x1403B3A12, 0x1403F7582), SELECT_VALUE(cmd_exec_stub_sp, cmd_exec_stub_mp),
			                  true);
			//Use empty memory to go to our stub first (can't do close jump, so need space for 12 bytes)

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
