#include <std_include.hpp>
#include "game.hpp"

namespace game
{
	int* keyCatchers;

	CmdArgs* cmd_args;
	CmdArgs* sv_cmd_args;
	cmd_function_s** cmd_functions;

	int* dvarCount;
	dvar_t** sortedDvars;

	PlayerKeyState* playerKeys;

	SOCKET* query_socket;

	const char** command_whitelist;

	namespace sp
	{
		gentity_s* g_entities;
	}

	namespace mp
	{
		cg_s* cgArray;

		gentity_s* g_entities;

		int* svs_numclients;
		client_t* svs_clients;

		std::uint32_t* sv_serverId_value;

		int* gameTime;
		int* serverTime;
	}

	int Cmd_Argc()
	{
		return cmd_args->argc[cmd_args->nesting];
	}

	const char* Cmd_Argv(int index)
	{
		return cmd_args->argv[cmd_args->nesting][index];
	}

	int SV_Cmd_Argc()
	{
		return sv_cmd_args->argc[sv_cmd_args->nesting];
	}

	const char* SV_Cmd_Argv(int index)
	{
		return sv_cmd_args->argv[sv_cmd_args->nesting][index];
	}

	namespace environment
	{
		launcher::mode mode = launcher::mode::none;

		launcher::mode get_mode()
		{
			if (mode == launcher::mode::none)
			{
				throw std::runtime_error("Launcher mode not valid. Something must be wrong.");
			}

			return mode;
		}

		bool is_dedi()
		{
			return get_mode() == launcher::mode::server;
		}

		bool is_mp()
		{
			return get_mode() == launcher::mode::multiplayer;
		}

		bool is_sp()
		{
			return get_mode() == launcher::mode::singleplayer;
		}

		void set_mode(const launcher::mode _mode)
		{
			mode = _mode;
		}

		void initialize()
		{
			keyCatchers = reinterpret_cast<int*>(SELECT_VALUE(0x1417CF6E0, 0x1419E1ADC));

			cmd_args = reinterpret_cast<CmdArgs*>(SELECT_VALUE(0x144CE7F70, 0x144518480));
			sv_cmd_args = reinterpret_cast<CmdArgs*>(SELECT_VALUE(0x144CE8020, 0x144518530));
			cmd_functions = reinterpret_cast<cmd_function_s**>(SELECT_VALUE(0x144CE80C8, 0x1445185D8));

			dvarCount = reinterpret_cast<int*>(SELECT_VALUE(0x1458CBA3C, 0x1478EADF4));
			sortedDvars = reinterpret_cast<dvar_t**>(SELECT_VALUE(0x1458CBA60, 0x1478EAE10));

			playerKeys = reinterpret_cast<PlayerKeyState*>(SELECT_VALUE(0x14164138C, 0x1419DEABC));

			query_socket = reinterpret_cast<SOCKET*>(SELECT_VALUE(0, 0x147AD1A78));

			command_whitelist = reinterpret_cast<const char**>(SELECT_VALUE(0x14086AA70, 0x1409E3AB0));

			if (is_sp())
			{
				sp::g_entities = reinterpret_cast<sp::gentity_s*>(0x143C91600);
			}
			else
			{
				mp::cgArray = reinterpret_cast<mp::cg_s*>(0x14176EC00);

				mp::g_entities = reinterpret_cast<mp::gentity_s*>(0x14427A0E0);

				mp::svs_numclients = reinterpret_cast<int*>(0x14647B28C);
				mp::svs_clients = reinterpret_cast<mp::client_t*>(0x14647B290);

				mp::sv_serverId_value = reinterpret_cast<std::uint32_t*>(0x144DF9478);

				mp::gameTime = reinterpret_cast<int*>(0x1443F4B6C);
				mp::serverTime = reinterpret_cast<int*>(0x14647B280);
			}
		}
	}
}
