#include <std_include.hpp>
#include "lua_script.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

namespace scripting::lua
{
	void test()
	{
		sol::state lua_state;
		lua_state.open_libraries(sol::lib::base);
		lua_state.script("print('hello world')");
	}
}
