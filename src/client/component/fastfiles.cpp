#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include "command.hpp"
#include "console.hpp"

#include <utils/hook.hpp>
#include <utils/memory.hpp>

namespace fastfiles
{
	namespace
	{
		utils::hook::detour db_try_load_x_file_internal_hook;

		void db_try_load_x_file_internal(const char* zoneName, const int zone_flags, const int is_base_map)
		{
			console::info("Loading fastfile %s\n", zoneName);
			return db_try_load_x_file_internal_hook.invoke<void>(zoneName, zone_flags, is_base_map);
		}

		void reallocate_asset_pool(const game::XAssetType type, const unsigned int new_size)
		{
			const size_t element_size = game::DB_GetXAssetTypeSize(type);

			auto* new_pool = utils::memory::get_allocator()->allocate(new_size * element_size);
			std::memmove(new_pool, game::DB_XAssetPool[type], game::g_poolSize[type] * element_size);

			game::DB_XAssetPool[type] = new_pool;
			game::g_poolSize[type] = new_size;
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
					console::info("usage: loadzone <zone>\n");
					return;
				}

				game::XZoneInfo info;
				info.name = params.get(1);
				info.allocFlags = 1;
				info.freeFlags = 0;

				game::DB_LoadXAssets(&info, 1, game::DBSyncMode::DB_LOAD_SYNC);
			});

			command::add("materiallist", [](const command::params& params)
			{
				game::DB_EnumXAssets_FastFile(game::ASSET_TYPE_MATERIAL, [](const game::XAssetHeader header, void*)
				{
					if(header.material && header.material->name)
					{
						console::info("%s\n", header.material->name);
					}
				}, nullptr, false);
			});

			if (!game::environment::is_sp())
			{
				reallocate_asset_pool(game::ASSET_TYPE_WEAPON, 320);
			}
		}
	};
}

REGISTER_COMPONENT(fastfiles::component)
