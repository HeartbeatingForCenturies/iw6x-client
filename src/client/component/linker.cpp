#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

namespace linker
{
	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (!game::environment::is_linker())
			{
				return;
			}


		}
	};
}

REGISTER_COMPONENT(linker::component)
