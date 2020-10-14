#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "server_list.hpp"
#include "game_console.hpp"
#include "command.hpp"
#include "localized_strings.hpp"
#include "game/game.hpp"

#include "utils/string.hpp"
#include "utils/hook.hpp"

namespace server_list
{
	namespace
	{
		struct server_info
		{
			// gotta add more to this
			char clients;
			char max_clients;
			short ping;
			std::string host_name;
			std::string map_name;
			std::string game_type;
			char in_game;
		};

		bool update_server_list = false;

		std::mutex mutex;
		std::vector<server_info> servers;

		void lui_open_menu_stub(int /*controllerIndex*/, const char* /*menu*/, int /*a3*/, int /*a4*/,
		                        unsigned int /*a5*/)
		{
			game::Cmd_ExecuteSingleCommand(0, 0, "lui_open menu_systemlink_join\n");
		}

		void refresh_server_list()
		{
			// refresh server list here
		}

		void join_server(int, int, const int index)
		{
			printf("Join %d ...\n", index);
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

		void add_server()
		{
			server_info server;

			server.host_name = utils::string::va("^%dIW6x Test Server %d", servers.size() + 1, servers.size());
			server.map_name = "mp_favela_iw6";
			server.game_type = "war";

			server.clients = 0;
			server.max_clients = 18;
			server.ping = static_cast<short>(rand()) % 999;

			server.in_game = 0;

			insert_server(std::move(server));

			update_server_list = true;
		}
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

			command::add("addTestServer", [&]()
			{
				add_server();
			});

			command::add("clearTestServers", [&]()
			{
				std::lock_guard<std::mutex> _(mutex);
				servers.clear();
			});

			command::add("lui_open", [](command::params params)
			{
				if (params.size() <= 1)
				{
					game_console::print(7, "usage: lui_open <name>\n");
					return;
				}

				game::LUI_OpenMenu(0, params[1], 1, 0, 0);
			});
		}
	};
}

REGISTER_MODULE(server_list::module)
