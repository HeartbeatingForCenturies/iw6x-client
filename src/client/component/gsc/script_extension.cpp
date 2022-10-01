#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include <utils/hook.hpp>

#include "component/console.hpp"

namespace gsc
{
	namespace
	{
		void scr_print()
		{
			for (auto i = 0u; i < game::Scr_GetNumParam(); ++i)
			{
				console::info("[script]: %s\n", game::Scr_GetString(i));
			}
		}
	}

	class extension final : public component_interface
	{
	public:
		void post_unpack() override
		{
			utils::hook::set<void(*)()>(SELECT_VALUE(0x14086F468, 0x1409E6CE8), scr_print);
		}
	};
}

REGISTER_COMPONENT(gsc::extension)
