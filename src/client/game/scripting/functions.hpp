#pragma once
#include "game/game.hpp"

namespace scripting
{
	using script_function = void(*)(game::scr_entref_t);

	script_function find_function(const std::string& name, const bool prefer_global);
}
