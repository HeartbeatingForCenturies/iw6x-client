#include <std_include.hpp>
#include "game.hpp"

namespace game
{
	namespace native
	{
		
	}

	launcher::mode mode = launcher::mode::none;

	launcher::mode get_mode()
	{
		if (mode == launcher::mode::none)
		{
			throw std::runtime_error("Launcher mode not valid. Something must be wrong.");
		}

		return mode;
	}

	bool is_mp()
	{
		return get_mode() == launcher::mode::multiplayer;
	}

	bool is_sp()
	{
		return get_mode() == launcher::mode::singleplayer;
	}

	bool is_dedi()
	{
		return get_mode() == launcher::mode::server;
	}

	void initialize(const launcher::mode _mode)
	{
		mode = _mode;
	}
}
