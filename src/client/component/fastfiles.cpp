#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include "command.hpp"
#include "game_console.hpp"

#include <utils/hook.hpp>

namespace fastfiles
{
	namespace
	{
		utils::hook::detour db_try_load_x_file_internal_hook;

		void db_try_load_x_file_internal(const char* zoneName, int zoneFlags, int isBaseMap)
		{
			game_console::print(game_console::con_type_info, "Loading fastfile %s\n", zoneName);
			return db_try_load_x_file_internal_hook.invoke<void>(zoneName, zoneFlags, isBaseMap);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			db_try_load_x_file_internal_hook.create(
				SELECT_VALUE(0x140275850, 0x1403237F0), &db_try_load_x_file_internal);

			command::add("loadzone", [](const command::params& params)
			{
				if (params.size() < 2)
				{
					game_console::print(game_console::con_type_info, "usage: loadzone <zone>\n");
					return;
				}

				game::XZoneInfo info;
				info.name = params.get(1);
				info.allocFlags = 1;
				info.freeFlags = 0;

				game::DB_LoadXAssets(&info, 1, game::DBSyncMode::DB_LOAD_SYNC);
			});
		}
	};
}

REGISTER_COMPONENT(fastfiles::component)
