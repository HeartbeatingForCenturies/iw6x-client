#include <std_include.hpp>
#include "context.hpp"
#include "value_conversion.hpp"

#include "../execution.hpp"

namespace scripting::lua
{
	namespace
	{
		void setup_entity_type(sol::state& state)
		{
			state["level"] = entity{*game::levelEntityId};

			auto entity_type = state.new_usertype<entity>("entity");
			entity_type["set"] = [](const entity& entity, const std::string& field,
			                        const sol::lua_value& value)
			{
				entity.set(field, convert(value));
			};

			entity_type["get"] = [](const entity& entity, const sol::this_state s, const std::string& field)
			{
				return convert(s, entity.get(field));
			};

			entity_type["notify"] = [](const entity& entity, const std::string& event, sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for(auto arg : va)
				{
					arguments.push_back(convert(arg));
				}
				
				notify(entity, event, arguments);
			};

			entity_type["call"] = [](const entity& entity, const sol::this_state s, const std::string& function, sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for(auto arg : va)
				{
					arguments.push_back(convert(arg));
				}
				
				return convert(s, entity.call(function, arguments));
			};
		}
	}

	context::context(const std::string& file)
		: scheduler_(state_)
	{
		this->state_.open_libraries(sol::lib::base, sol::lib::package);
		setup_entity_type(this->state_);

		this->state_.safe_script_file(file);
	}

	context::~context()
	{
		this->scheduler_.clear();
		this->state_ = {};
	}

	void context::run_frame()
	{
		scheduler_.run_frame();
	}

	void context::notify(const event& /*e*/)
	{
	}
}
