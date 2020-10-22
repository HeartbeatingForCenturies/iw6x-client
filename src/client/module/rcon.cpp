#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "utils/hook.hpp"
#include "utils/string.hpp"
#include "game/game.hpp"

#include "command.hpp"
#include "game_console.hpp"
#include "network.hpp"
#include "scheduler.hpp"

namespace rcon
{
	namespace
	{
		bool is_redirecting = false;
		game::netadr_s redirect_target = {};
		std::recursive_mutex redirect_lock;
		utils::hook::detour print_hook;

		void setup_redirect(const game::netadr_s& target)
		{
			std::lock_guard<std::recursive_mutex> $(redirect_lock);

			rcon::is_redirecting = true;
			rcon::redirect_target = target;
		}

		void clear_redirect()
		{
			std::lock_guard<std::recursive_mutex> $(redirect_lock);

			rcon::is_redirecting = false;
			rcon::redirect_target = {};
		}

		int __cdecl print_stub(const char* fmt, ...)
		{
			std::lock_guard<std::recursive_mutex> $(redirect_lock);

			static thread_local utils::string::va_provider<8, 256> provider;
			va_list ap;
			va_start(ap, fmt);
			const char* result = provider.get(fmt, ap);
			va_end(ap);

			if (is_redirecting)
			{
				network::send(redirect_target, "print\n", result);
			}
			else
			{
				print_hook.invoke<int>(result);
			}

			return 0;
		}
	}

	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_sp()) return;

			scheduler::once([]()
			{
				game::Dvar_RegisterString("rcon_password", "", game::DvarFlags::DVAR_FLAG_NONE, "The password for remote console");
			}, scheduler::pipeline::main);

			command::add("status", [&](command::params& params)
			{
				auto dvar = game::Dvar_FindVar("sv_running");
				auto sv_maxclients = game::Dvar_FindVar("sv_maxclients");
				auto mapname = game::Dvar_FindVar("mapname");

				if (!dvar || !sv_maxclients || !mapname || !dvar->current.enabled)
				{
					game_console::print(game_console::con_type_error, "Server is not running\n");
					return;
				}

				auto status_buffer = ""s;

				status_buffer.append(utils::string::va("map: %s\n", mapname->current.string));
				status_buffer.append("num score bot ping guid                             name             address               qport\n");
				status_buffer.append("--- ----- --- ---- -------------------------------- ---------------- --------------------- -----\n");

				for (int i = 0; i < sv_maxclients->current.integer; i++)
				{
					auto client = &game::mp::svs_clients[i];
					auto self = &game::mp::g_entities[i];

					char clean_name[32] = { 0 };
					strncpy_s(clean_name, self->client->sess.cs.name, 32);
					game::I_CleanStr(clean_name);

					if (client->header.state >= 1 && self && self->client)
					{
						status_buffer.append(utils::string::va("%3i %5i %3s %s %32s %16s %21s %5i\n",
							i,
							self->client->sess.scores.score,
							game::SV_BotIsBot(i) ? "Yes" : "No",
							(client->header.state == 2) ? "CNCT" : (client->header.state == 1) ? "ZMBI" : utils::string::va("%4i", client->ping),
							game::SV_GetGuid(i),
							clean_name,
							network::net_adr_to_string(client->header.netchan.remoteAddress),
							client->header.netchan.remoteAddress.port)
						);
					}
				}

				if (game::environment::is_dedi())
				{
					printf("%s\n", status_buffer.data());
				}
				else
				{
					game_console::print(game_console::con_type_info, status_buffer.data());
				}
			});

			if (!game::environment::is_dedi())
			{
				command::add("rcon", [&](command::params& params)
				{
					static std::string rcon_password{};

					if (params.size() < 2) return;

					auto operation = params.get(1);
					if (operation == "login"s)
					{
						if (params.size() < 3)
							return;

						rcon_password = params.get(2);
					}
					else if (operation == "logout"s)
					{
						rcon_password.clear();
					}
					else
					{
						// If you are the server, don't bother with rcon and just execute the command
						if (game::Dvar_FindVar("sv_running")->current.enabled)
						{
							game::Cbuf_AddText(0, params.join(1).data());
							return;
						}

						if (rcon_password.empty())
						{
							game_console::print(game_console::con_type_warning, "You must login first to use RCON");
							return;
						}

						if (*reinterpret_cast<std::int32_t*>(0x1419E1AE0) >= 5)
						{
							const auto target = *reinterpret_cast<game::netadr_s*>(0x141CB535C);
							const auto data = rcon_password + " " + params.join(1);
							network::send(target, "rcon", data);
						}
						else
						{
							game_console::print(game_console::con_type_warning, "You need to be connected to a server!\n");
						}
					}
				});
			}
			else
			{
				// setup printf hook for redirecting data
				print_hook.create(reinterpret_cast<void*>(printf), &print_stub);

				network::on("rcon", [](const game::netadr_s& addr, const std::string_view& data)
				{
					const auto message = std::string{ data };
					auto pos = message.find_first_of(" ");
					if (pos == std::string::npos)
					{
						printf("Invalid RCon request from %s\n", network::net_adr_to_string(addr));
						return;
					}

					auto password = message.substr(0, pos);
					auto command = message.substr(pos + 1);
					auto rcon_password = game::Dvar_FindVar("rcon_password");
					if (command.empty() || !rcon_password || !rcon_password->current.string || !strlen(rcon_password->current.string))
					{
						return;
					}

					setup_redirect(addr);

					if (password != rcon_password->current.string)
					{
						printf("Invalid rcon password\n");
					}
					else
					{
						command::execute(command, true);
					}

					clear_redirect();
				});
			}
		}
	};
}

REGISTER_MODULE(rcon::module)
