#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "command.hpp"
#include "game/game.hpp"
#include "utils/hook.hpp"

namespace stats
{
	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			if (!game::environment::is_mp())
			{
				return;
			}

			command::add("unlockall", []()
			{
				utils::hook::set<BYTE>(0x1445A3798, 0x0A); // Prestige
				utils::hook::set<short>(0x1445A34A0, 5000); // squad points

				// unlocks all camos
				for (int i = 0; i < 0x2D0; i++)
				{
					utils::hook::set<short>(0x1445A2B40 + i, 0xFF);
				}

				//squad member ranks
				for (long long i = 0; i < 10; i++)
				{
					utils::hook::set<int>(0x14459F857 + (0x564 * i), 4805);
				}

				//squad members unlocked
				for (int i = 0; i < 9; i++)
				{
					utils::hook::set<short>(0x14459FD97 + (0x564 * i), 0x0100);
				}

				//only Extinction
				utils::hook::set<short>(0x1445A6B62, 9999); // Teeth
				utils::hook::set<BYTE>(0x1445A5F96, 25); // Prestige
				utils::hook::set<short>(0x1445A5F90, 27); // level
			});
		}
	};
}

REGISTER_MODULE(stats::module)
