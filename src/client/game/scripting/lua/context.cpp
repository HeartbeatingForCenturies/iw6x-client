#include <std_include.hpp>
#include "context.hpp"

namespace scripting::lua
{
	namespace
	{
		script_value convert(const sol::lua_value& value)
		{
			if(value.is<int>())
			{
				return { value.as<int>() };
			}

			return {};
		}

		sol::lua_value convert(sol::this_state& state, const script_value& value)
		{
			if(value.is<int>())
			{
				return {state, value.as<int>()};
			}

			return {};
		}

		void setup_entity_type(sol::state& state)
		{
			auto entity_type = state.new_usertype<entity>("entity");
			entity_type["set"] = [](const entity& entity, sol::this_state s, const std::string& field, const sol::lua_value& value)
			{
				//entity.set(field, convert(value));
				printf("Converting\n");
				return convert(s, convert(value));
			};

			// TODO: Actually get the level entity
			state["level"] = entity();
		}
	}

	context::context(const std::string& file)
	{
		this->state_.open_libraries(sol::lib::base, sol::lib::package);
		setup_entity_type(this->state_);

		// TODO: Setup context here
		this->state_.set_function("lul", [](const std::function<void()>& callback)
		{
			printf("This is lul\n");
			callback();
		});

		this->state_.safe_script_file(file);
	}

	context::~context()
	{
		this->state_ = {};
	}

	void context::run_frame()
	{
	}

	void context::notify(const event& /*e*/)
	{
	}
}
