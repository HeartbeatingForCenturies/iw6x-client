#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "command.hpp"

#include "game/game.hpp"
#include "utils/hook.hpp"
#include "utils/string.hpp"
#include "utils/memory.hpp"

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

			auto inq = 0;
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

	int params::size()
	{
		return game::Cmd_Argc();
	}

	const char* params::get(int index)
	{
		return game::Cmd_Argv(index);
	}

	std::string params::join(int index)
	{
		std::string result = {};

		for (int i = index; i < this->size(); i++)
		{
			if (i > index) result.append(" ");
			result.append(this->get(i));
		}
		return result;
	}

	int params_sv::size()
	{
		return game::SV_Cmd_Argc();
	}

	const char* params_sv::get(const int index)
	{
		return game::SV_Cmd_Argv(index);
	}

	std::string params_sv::join(const int index)
	{
		std::string result = {};

		for (int i = index; i < this->size(); i++)
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

	void add(const char* name, const std::function<void(params&)>& callback)
	{
		const auto command = utils::string::to_lower(name);

		if (handlers.find(command) == handlers.end())
			add_raw(name, main_handler);

		handlers[command] = callback;
	}

	void add(const char* name, const std::function<void()>& callback)
	{
		add(name, [callback](params&)
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
			handlers_sv[command] = callback;
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

	enum he_type_t
	{
		HE_TYPE_FREE = 0x0,
		HE_TYPE_TEXT = 0x1,
		HE_TYPE_VALUE = 0x2,
		HE_TYPE_PLAYERNAME = 0x3,
		HE_TYPE_MATERIAL = 0x4,
		HE_TYPE_TIMER_DOWN = 0x5,
		HE_TYPE_TIMER_UP = 0x6,
		HE_TYPE_TIMER_STATIC = 0x7,
		HE_TYPE_TENTHS_TIMER_DOWN = 0x8,
		HE_TYPE_TENTHS_TIMER_UP = 0x9,
		HE_TYPE_TENTHS_TIMER_STATIC = 0xA,
		HE_TYPE_CLOCK_DOWN = 0xB,
		HE_TYPE_CLOCK_UP = 0xC,
		HE_TYPE_WAYPOINT = 0xD,
		HE_TYPE_COUNT = 0xE,
	};

	struct $C96EA5EC2ACBB9C0BF22693F316ACC67
	{
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	};

	/* 1446 */
	union hudelem_color_t
	{
		$C96EA5EC2ACBB9C0BF22693F316ACC67 _s0;
		int rgba;
	};

	struct hudelem_s
	{
		he_type_t type;
		float x;
		float y;
		float z;
		int targetEntNum;
		float fontScale;
		float fromFontScale;
		int fontScaleStartTime;
		int fontScaleTime;
		int font;
		int alignOrg;
		int alignScreen;
		hudelem_color_t color;
		hudelem_color_t fromColor;
		int fadeStartTime;
		int fadeTime;
		int label;
		int width;
		int height;
		int materialIndex;
		int fromWidth;
		int fromHeight;
		int scaleStartTime;
		int scaleTime;
		float fromX;
		float fromY;
		int fromAlignOrg;
		int fromAlignScreen;
		int moveStartTime;
		int moveTime;
		int time;
		int duration;
		float value;
		int text;
		float sort;
		hudelem_color_t glowColor;
		int fxBirthTime;
		int fxLetterTime;
		int fxDecayStartTime;
		int fxDecayDuration;
		int soundID;
		int flags;
	};

	class module final : public module_interface
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

				if (game::environment::is_mp())
				{
					add_mp_commands();

					add("hudelem", [&]()
					{
						auto text = "Mod support in IW6x?";;
						auto index = ((int(*)(const char*, size_t, size_t,size_t, const char*))0x140161F90)(text, 541, 0x258u, 1, "");

						auto hudelem = ((hudelem_s*(*)(int, int))0x1403997E0)(0x7FF, 0);
						hudelem->fontScale = 2.0f;
						hudelem->x = 100;
						hudelem->color.rgba = 0xFF000000;
						hudelem->glowColor.rgba = 0xFF00FFAE;
						hudelem->text = index;
						hudelem->type = HE_TYPE_TEXT;
						hudelem->flags |= 1;


						text = "^1Not ^2yet, ^3but ^5soon!";
						index = ((int(*)(const char*, size_t, size_t,size_t, const char*))0x140161F90)(text, 541, 0x258u, 1, "");

						hudelem = ((hudelem_s*(*)(int, int))0x1403997E0)(0x7FF, 0);
						hudelem->fontScale = 2.0f;
						hudelem->x = 100;
						hudelem->y = 50;
						hudelem->color.rgba = 0xFFFFFFFF;
						hudelem->text = index;
						hudelem->type = HE_TYPE_TEXT;
						hudelem->flags |= 1;
						
						//auto x = *((size_t*)0x14086F810 + 7 * 0);
						//*(DWORD*)(hudelem + x) = index;
					});
				}
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
		}

		void add_mp_commands()
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

				game::mp::g_entities[client_num].client->ps.delta_angles[0] = std::strtof(params.get(1), nullptr);
				game::mp::g_entities[client_num].client->ps.delta_angles[1] = std::strtof(params.get(2), nullptr);
				game::mp::g_entities[client_num].client->ps.delta_angles[2] = std::strtof(params.get(3), nullptr);
			});
		}
	};
}

REGISTER_MODULE(command::module)
