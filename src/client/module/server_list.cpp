#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "server_list.hpp"
#include "game_console.hpp"
#include "command.hpp"
#include "localized_strings.hpp"
#include "network.hpp"
#include "scheduler.hpp"
#include "party.hpp"
#include "game/game.hpp"

#include "utils/cryptography.hpp"
#include "utils/string.hpp"
#include "utils/hook.hpp"

namespace server_list
{
	namespace
	{
		struct server_info
		{
			// gotta add more to this
			int clients;
			int max_clients;
			int ping;
			std::string host_name;
			std::string map_name;
			std::string game_type;
			char in_game;
			game::netadr_s address;
		};

		struct
		{
			game::netadr_s address;
			volatile bool requesting = false;
			std::unordered_map<game::netadr_s, int> queued_servers;
		} master_state;

		volatile bool update_server_list = false;

		std::mutex mutex;
		std::vector<server_info> servers;

		void lui_open_menu_stub(int /*controllerIndex*/, const char* /*menu*/, int /*a3*/, int /*a4*/,
		                        unsigned int /*a5*/)
		{
			game::Cmd_ExecuteSingleCommand(0, 0, "lui_open menu_systemlink_join\n");
		}

		void refresh_server_list()
		{
			{
				std::lock_guard<std::mutex> _(mutex);
				servers.clear();
				master_state.queued_servers.clear();
			}

			if (get_master_server(master_state.address))
			{
				master_state.requesting = true;
				network::send(master_state.address, "getservers", utils::string::va("IW6 %i full empty", PROTOCOL));
			}
		}

		void join_server(int, int, const int index)
		{
			printf("Join %d ...\n", index);

			auto i = static_cast<size_t>(index);
			std::lock_guard<std::mutex> _(mutex);

			if (i < servers.size())
			{
				party::connect(servers[i].address);
			}
		}

		bool server_list_refresher()
		{
			if (update_server_list)
			{
				update_server_list = false;
			}

			return false;
		}

		int ui_feeder_count()
		{
			std::lock_guard<std::mutex> _(mutex);
			return static_cast<int>(servers.size());
		}

		const char* ui_feeder_item_text(int /*localClientNum*/, void* /*a2*/, void* /*a3*/, const size_t index,
		                                const size_t column)
		{
			std::lock_guard<std::mutex> _(mutex);

			if (index >= servers.size())
			{
				return "";
			}

			if (column == 0)
			{
				return utils::string::va("%s\n", servers[index].host_name.data());
			}

			if (column == 1)
			{
				return utils::string::va("%s\n", servers[index].map_name.data());
			}

			if (column == 2)
			{
				return utils::string::va("%d/%d", servers[index].clients, servers[index].max_clients);
			}

			if (column == 3)
			{
				return utils::string::va("%s\n", servers[index].game_type.data());
			}

			return "";
		}

		void insert_server(server_info&& server)
		{
			std::lock_guard<std::mutex> _(mutex);
			servers.emplace_back(std::move(server));
		}

		void do_frame_work()
		{
			auto& queue = master_state.queued_servers;
			if (queue.empty())
			{
				return;
			}

			std::lock_guard<std::mutex> _(mutex);

			size_t queried_servers = 0;
			const size_t query_limit = 3;

			for (auto i = queue.begin(); i != queue.end();)
			{
				if (i->second)
				{
					const auto now = game::Sys_Milliseconds();
					if (now - i->second > 10'000)
					{
						i = queue.erase(i);
						continue;
					}
				}
				else if (queried_servers++ < query_limit)
				{
					i->second = game::Sys_Milliseconds();
					network::send(i->first, "getInfo", utils::cryptography::random::get_challenge());
				}

				++i;
			}
		}
	}

	bool get_master_server(game::netadr_s& address)
	{
		return game::NET_StringToAdr("49.12.101.14:20810", &address);
	}

	void handle_info_response(const game::netadr_s& address, const utils::info_string& info)
	{
		int start_time{};
		const auto now = game::Sys_Milliseconds();

		{
			std::lock_guard<std::mutex> _(mutex);
			const auto entry = master_state.queued_servers.find(address);

			if (entry == master_state.queued_servers.end() || !entry->second)
			{
				return;
			}

			start_time = entry->second;
			master_state.queued_servers.erase(entry);
		}

		server_info server{};
		server.address = address;
		server.host_name = info.get("hostname");
		server.map_name = game::UI_LocalizeMapname(info.get("mapname").data());
		server.game_type = game::UI_LocalizeGametype(info.get("gametype").data());
		server.clients = atoi(info.get("clients").data());
		server.max_clients = atoi(info.get("sv_maxclients").data());
		server.ping = now - start_time;

		server.in_game = 1;

		if(server.host_name.size() > 30)
		{
			server.host_name.resize(30);
		}

		insert_server(std::move(server));

		update_server_list = true;
	}

	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			if (!game::environment::is_mp()) return;

			localized_strings::override("PLATFORM_SYSTEM_LINK_TITLE", "SERVER LIST");
			localized_strings::override("LUA_MENU_STORE_CAPS", "SERVER LIST");
			localized_strings::override("LUA_MENU_STORE_DESC", "Browse available servers.");

			// hook LUI_OpenMenu to show server list instead of store popup
			utils::hook::call(0x1404FE840, &lui_open_menu_stub);

			// refresh server list when needed
			utils::hook::call(0x1402F7480, &server_list_refresher);

			// replace UI_RunMenuScript call in LUI_CoD_LuaCall_RefreshServerList to our refresh_servers
			utils::hook::call(0x1401E7171, &refresh_server_list);
			utils::hook::call(0x1401E7616, &join_server);
			utils::hook::nop(0x1401E7635, 5);

			// do feeder stuff
			utils::hook::call(0x1401E7225, &ui_feeder_count);
			utils::hook::call(0x1401E7405, &ui_feeder_item_text);

			command::add("lui_open", [](command::params params)
			{
				if (params.size() <= 1)
				{
					game_console::print(7, "usage: lui_open <name>\n");
					return;
				}

				game::LUI_OpenMenu(0, params[1], 1, 0, 0);
			});

			scheduler::loop(do_frame_work, scheduler::pipeline::main);

			network::on("getServersResponse", [](const game::netadr_s& target, const std::string_view& data)
			{
				{
					std::lock_guard<std::mutex> _(mutex);
					if (!master_state.requesting || master_state.address != target)
					{
						return;
					}

					master_state.requesting = false;

					std::optional<size_t> start{};
					for (size_t i = 0; i + 6 < data.size(); ++i)
					{
						if (data[i + 6] == '\\')
						{
							start.emplace(i);
							break;
						}
					}

					if (!start.has_value())
					{
						return;
					}

					for (auto i = start.value(); i + 6 < data.size(); i += 7)
					{
						if (data[i + 6] != '\\')
						{
							break;
						}

						game::netadr_s address{};
						address.type = game::NA_IP;
						address.localNetID = game::NS_CLIENT1;
						memcpy(&address.ip[0], data.data() + i + 0, 4);
						memcpy(&address.port, data.data() + i + 4, 2);

						master_state.queued_servers[address] = 0;
					}
				}
			});
		}
	};
}

REGISTER_MODULE(server_list::module)
