#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "command.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>
#include <utils/memory.hpp>

namespace command
{
	namespace
	{
		utils::hook::detour client_command_hook;

		std::unordered_map<std::string, std::function<void(params&)>> handlers;
		std::unordered_map<std::string, std::function<void(int, params_sv&)>> handlers_sv;

		void main_handler()
		{
			params params = {};

			const auto command = utils::string::to_lower(params[0]);
			if (handlers.find(command) != handlers.end())
			{
				handlers[command](params);
			}
		}

		void client_command(const int clientNum, void* a2)
		{
			params_sv params = {};

			const auto command = utils::string::to_lower(params[0]);
			if (handlers_sv.find(command) != handlers_sv.end())
			{
				handlers_sv[command](clientNum, params);
			}

			client_command_hook.invoke<void>(clientNum, a2);
		}

		// Shamelessly stolen from Quake3
		// https://github.com/id-Software/Quake-III-Arena/blob/dbe4ddb10315479fc00086f08e25d968b4b43c49/code/qcommon/common.c#L364
		void parse_command_line()
		{
			static auto parsed = false;
			if (parsed)
			{
				return;
			}

			static std::string comand_line_buffer = GetCommandLineA();
			char* command_line = comand_line_buffer.data();

			auto& com_num_console_lines = *reinterpret_cast<int*>(0x1445CFF98);
			auto* com_console_lines = reinterpret_cast<char**>(0x1445CFFA0);

			auto inq = false;
			com_console_lines[0] = command_line;
			com_num_console_lines = 0;

			while (*command_line)
			{
				if (*command_line == '"')
				{
					inq = !inq;
				}
				// look for a + separating character
				// if commandLine came from a file, we might have real line seperators
				if ((*command_line == '+' && !inq) || *command_line == '\n' || *command_line == '\r')
				{
					if (com_num_console_lines == 0x20) // MAX_CONSOLE_LINES
					{
						break;
					}
					com_console_lines[com_num_console_lines] = command_line + 1;
					com_num_console_lines++;
					*command_line = '\0';
				}
				command_line++;
			}
			parsed = true;
		}

		void parse_commandline_stub()
		{
			parse_command_line();
			reinterpret_cast<void(*)()>(0x140413080)();
		}
	}

	params::params()
		: nesting_(game::cmd_args->nesting)
	{
	}

	int params::size() const
	{
		return game::cmd_args->argc[this->nesting_];
	}

	const char* params::get(const int index) const
	{
		if (index >= this->size())
		{
			return "";
		}

		return game::cmd_args->argv[this->nesting_][index];
	}

	std::string params::join(const int index) const
	{
		std::string result = {};

		for (auto i = index; i < this->size(); i++)
		{
			if (i > index) result.append(" ");
			result.append(this->get(i));
		}
		return result;
	}

	params_sv::params_sv()
		: nesting_(game::sv_cmd_args->nesting)
	{
	}

	int params_sv::size() const
	{
		return game::sv_cmd_args->argc[this->nesting_];
	}

	const char* params_sv::get(const int index) const
	{
		if (index >= this->size())
		{
			return "";
		}

		return game::sv_cmd_args->argv[this->nesting_][index];
	}

	std::string params_sv::join(const int index) const
	{
		std::string result = {};

		for (auto i = index; i < this->size(); i++)
		{
			if (i > index) result.append(" ");
			result.append(this->get(i));
		}
		return result;
	}

	void add_raw(const char* name, void (*callback)())
	{
		game::Cmd_AddCommandInternal(name, callback, utils::memory::get_allocator()->allocate<game::cmd_function_s>());
	}

	void add(const char* name, const std::function<void(const params&)>& callback)
	{
		const auto command = utils::string::to_lower(name);

		if (handlers.find(command) == handlers.end())
			add_raw(name, main_handler);

		handlers[command] = callback;
	}

	void add(const char* name, const std::function<void()>& callback)
	{
		add(name, [callback](const params&)
		{
			callback();
		});
	}

	void add_sv(const char* name, std::function<void(int, params_sv&)> callback)
	{
		// doing this so the sv command would show up in the console
		add_raw(name, nullptr);

		const auto command = utils::string::to_lower(name);

		if (handlers_sv.find(command) == handlers_sv.end())
			handlers_sv[command] = std::move(callback);
	}

	void execute(std::string command, const bool sync)
	{
		command += "\n";

		if (sync)
		{
			game::Cmd_ExecuteSingleCommand(0, 0, command.data());
		}
		else
		{
			game::Cbuf_AddText(0, command.data());
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_sp())
			{
				add_sp_commands();
			}
			else
			{
				utils::hook::call(0x14041213C, &parse_commandline_stub);

				add_mp_commands();
			}
		}

	private:
		void add_sp_commands()
		{
			add("noclip", [&]()
			{
				if (!game::SV_Loaded())
				{
					return;
				}

				game::sp::g_entities[0].client->flags ^= 1;
				game::CG_GameMessage(0, utils::string::va("noclip %s",
				                                          game::sp::g_entities[0].client->flags & 1
					                                          ? "^2on"
					                                          : "^1off"));
			});

			add("ufo", [&]()
			{
				if (!game::SV_Loaded())
				{
					return;
				}

				game::sp::g_entities[0].client->flags ^= 2;
				game::CG_GameMessage(
					0, utils::string::va("ufo %s", game::sp::g_entities[0].client->flags & 2 ? "^2on" : "^1off"));
			});

			add("give", [](const params& params)
			{
				if (!game::SV_Loaded())
				{
					return;
				}

				if (params.size() < 2)
				{
					game::CG_GameMessage(0, "You did not specify a weapon name");
					return;
				}

				auto ps = game::SV_GetPlayerstateForClientNum(0);
				auto wp = game::G_GetWeaponForName(params.get(1));
				if (game::G_GivePlayerWeapon(ps, wp, 0, 0, 0))
				{
					game::G_InitializeAmmo(ps, wp, 0);
					game::G_SelectWeapon(0, wp);
				}
			});

			add("take", [](const params& params)
			{
				if (!game::SV_Loaded())
				{
					return;
				}

				if (params.size() < 2)
				{
					game::CG_GameMessage(0, "You did not specify a weapon name");
					return;
				}

				auto ps = game::SV_GetPlayerstateForClientNum(0);
				auto wp = game::G_GetWeaponForName(params.get(1));
				game::G_TakePlayerWeapon(ps, wp);
			});
		}

		static void add_mp_commands()
		{
			client_command_hook.create(0x1403929B0, &client_command);

			add_sv("god", [&](const int client_num, params_sv&)
			{
				if (!game::Dvar_FindVar("sv_cheats")->current.enabled)
				{
					game::SV_GameSendServerCommand(client_num, 1, "f \"Cheats are not enabled on this server\"");
					return;
				}

				game::mp::g_entities[client_num].flags ^= 1;
				game::SV_GameSendServerCommand(client_num, 1,
				                               utils::string::va("f \"godmode %s\"",
				                                                 game::mp::g_entities[client_num].flags & 1
					                                                 ? "^2on"
					                                                 : "^1off"));
			});

			add_sv("noclip", [&](const int client_num, params_sv&)
			{
				if (!game::Dvar_FindVar("sv_cheats")->current.enabled)
				{
					game::SV_GameSendServerCommand(client_num, 1, "f \"Cheats are not enabled on this server\"");
					return;
				}

				game::mp::g_entities[client_num].client->flags ^= 1;
				game::SV_GameSendServerCommand(client_num, 1,
				                               utils::string::va("f \"noclip %s\"",
				                                                 game::mp::g_entities[client_num].client->flags & 1
					                                                 ? "^2on"
					                                                 : "^1off"));
			});

			add_sv("ufo", [&](const int client_num, params_sv&)
			{
				if (!game::Dvar_FindVar("sv_cheats")->current.enabled)
				{
					game::SV_GameSendServerCommand(client_num, 1, "f \"Cheats are not enabled on this server\"");
					return;
				}

				game::mp::g_entities[client_num].client->flags ^= 2;
				game::SV_GameSendServerCommand(client_num, 1,
				                               utils::string::va("f \"ufo %s\"",
				                                                 game::mp::g_entities[client_num].client->flags & 2
					                                                 ? "^2on"
					                                                 : "^1off"));
			});

			add_sv("setviewpos", [&](const int client_num, params_sv& params)
			{
				if (!game::Dvar_FindVar("sv_cheats")->current.enabled)
				{
					game::SV_GameSendServerCommand(client_num, 1, "f \"Cheats are not enabled on this server\"");
					return;
				}

				if (params.size() < 4)
				{
					game::SV_GameSendServerCommand(client_num, 1,
					                               "f \"You did not specify the correct number of coordinates\"");
					return;
				}

				game::mp::g_entities[client_num].client->ps.origin[0] = std::strtof(params.get(1), nullptr);
				game::mp::g_entities[client_num].client->ps.origin[1] = std::strtof(params.get(2), nullptr);
				game::mp::g_entities[client_num].client->ps.origin[2] = std::strtof(params.get(3), nullptr);
			});

			add_sv("setviewang", [&](const int client_num, params_sv& params)
			{
				if (!game::Dvar_FindVar("sv_cheats")->current.enabled)
				{
					game::SV_GameSendServerCommand(client_num, 1, "f \"Cheats are not enabled on this server\"");
					return;
				}

				if (params.size() < 4)
				{
					game::SV_GameSendServerCommand(client_num, 1,
					                               "f \"You did not specify the correct number of coordinates\"");
					return;
				}

				game::mp::g_entities[client_num].client->ps.delta_angles[0] = std::strtof(params.get(1), nullptr);
				game::mp::g_entities[client_num].client->ps.delta_angles[1] = std::strtof(params.get(2), nullptr);
				game::mp::g_entities[client_num].client->ps.delta_angles[2] = std::strtof(params.get(3), nullptr);
			});

			add_sv("give", [](const int client_num, params_sv& params)
			{
				if (!game::Dvar_FindVar("sv_cheats")->current.enabled)
				{
					game::SV_GameSendServerCommand(client_num, 1, "f \"Cheats are not enabled on this server\"");
					return;
				}

				if (params.size() < 2)
				{
					game::SV_GameSendServerCommand(client_num, 1, "f \"You did not specify a weapon name\"");
					return;
				}

				auto ps = game::SV_GetPlayerstateForClientNum(client_num);
				auto wp = game::G_GetWeaponForName(params.get(1));
				if (game::G_GivePlayerWeapon(ps, wp, 0, 0, 0))
				{
					game::G_InitializeAmmo(ps, wp, 0);
					game::G_SelectWeapon(client_num, wp);
				}
			});

			add_sv("take", [](const int client_num, params_sv& params)
			{
				if (!game::Dvar_FindVar("sv_cheats")->current.enabled)
				{
					game::SV_GameSendServerCommand(client_num, 1, "f \"Cheats are not enabled on this server\"");
					return;
				}

				if (params.size() < 2)
				{
					game::SV_GameSendServerCommand(client_num, 1, "f \"You did not specify a weapon name\"");
					return;
				}

				auto ps = game::SV_GetPlayerstateForClientNum(client_num);
				auto wp = game::G_GetWeaponForName(params.get(1));
				game::G_TakePlayerWeapon(ps, wp);
			});
		}
	};
}

REGISTER_COMPONENT(command::component)
