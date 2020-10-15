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

	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_mp())
			{
				add_mp_commands();
			}
			else if (game::environment::is_sp())
			{
				add_sp_commands();
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

			add_sv("noclip", [&](const int client_num, params_sv&)
			{
				if (!game::SV_Loaded())
				{
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
				if (!game::SV_Loaded())
				{
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
				if (!game::SV_Loaded())
				{
					return;
				}

				game::mp::g_entities[client_num].client->ps.origin[0] = std::strtof(params.get(1), nullptr);
				game::mp::g_entities[client_num].client->ps.origin[1] = std::strtof(params.get(2), nullptr);
				game::mp::g_entities[client_num].client->ps.origin[2] = std::strtof(params.get(3), nullptr);
			});

			add_sv("setviewang", [&](const int client_num, params_sv& params)
			{
				if (!game::SV_Loaded())
				{
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
