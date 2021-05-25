#pragma once
#include "game/game.hpp"

namespace party
{
	void reset_connect_state();

	void connect(const game::netadr_s& target);
	void start_map(const std::string& mapname);
	void map_restart();
}
