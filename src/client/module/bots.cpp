#include <std_include.hpp>

#include "command.hpp"
#include "game/game.hpp"

class bots final : public module
{
public:
	void post_unpack() override
	{
		if (game::environment::is_sp())
		{
			return;
		}

		command::add("spawnBot", [](command::params&)
		{
			game::SV_AddTestClient(0, rand() & 1, 0, 0);
		});
	}
};

REGISTER_MODULE(bots);
