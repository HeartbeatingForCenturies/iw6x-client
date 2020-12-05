#include <std_include.hpp>
#include "value_conversion.hpp"

namespace scripting::lua
{
	script_value convert(const sol::lua_value& value)
	{
		if (value.is<int>())
		{
			return {value.as<int>()};
		}

		if (value.is<unsigned int>())
		{
			return {value.as<unsigned int>()};
		}

		if (value.is<bool>())
		{
			return {value.as<bool>()};
		}

		if (value.is<double>())
		{
			return {value.as<double>()};
		}

		if (value.is<float>())
		{
			return {value.as<float>()};
		}

		if (value.is<std::string>())
		{
			return {value.as<std::string>()};
		}

		if (value.is<entity>())
		{
			return {value.as<entity>()};
		}

		// TODO: Convert vector
		
		return {};
	}

	sol::lua_value convert(lua_State* state, const script_value& value)
	{
		if (value.is<int>())
		{
			return {state, value.as<int>()};
		}

		if (value.is<float>())
		{
			return {state, value.as<float>()};
		}

		if (value.is<std::string>())
		{
			return {state, value.as<std::string>()};
		}

		if (value.is<entity>())
		{
			return {state, value.as<entity>()};
		}

		// TODO: Support vector

		return {};
	}
}
