#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include <utils/hook.hpp>
#include "game/game.hpp"

namespace security
{
	namespace
	{
		void set_cached_playerdata_stub(const int localclient, const int index1, const int index2)
		{
			if (index1 >= 0 && index1 < 18 && index2 >= 0 && index2 < 42)
			{
				reinterpret_cast<void(*)(int, int, int)>(0x1405834B0)(localclient, index1, index2);
			}
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_sp()) return;

			// Patch vulnerability in PlayerCards_SetCachedPlayerData
			utils::hook::call(0x140287C5C, set_cached_playerdata_stub);
		}
	};
}

REGISTER_COMPONENT(security::component)
