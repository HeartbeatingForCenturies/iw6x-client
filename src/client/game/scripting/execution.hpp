#pragma once
#include "game/game.hpp"
#include "entity.hpp"
#include "script_value.hpp"

namespace scripting
{
	script_value call_function(const std::string& name, const std::vector<script_value>& arguments);
	script_value call_function(const std::string& name, const entity& entity,
	                           const std::vector<script_value>& arguments);

	void set_entity_field(const entity& entity, const std::string& field, const script_value& value);
	script_value get_entity_field(const entity& entity, const std::string& field);
}
