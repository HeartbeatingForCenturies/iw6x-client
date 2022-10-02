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
				console::info("%s", game::Scr_GetString(i));
			}
		}

		void scr_print_ln()
		{
			for (auto i = 0u; i < game::Scr_GetNumParam(); ++i)
			{
				console::info("%s", game::Scr_GetString(i));
			}

			console::info("\n");
		}
	}

	class extension final : public component_interface
	{
	public:
		void post_unpack() override
		{
			utils::hook::set<void(*)()>(SELECT_VALUE(0x14086F468, 0x1409E6CE8), scr_print);
			utils::hook::set<void(*)()>(SELECT_VALUE(0x14086F480, 0x1409E6D00), scr_print_ln);
		}
	};
}

REGISTER_COMPONENT(gsc::extension)
