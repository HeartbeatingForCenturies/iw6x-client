#include <std_include.hpp>
#include "context.hpp"

namespace scripting::lua
{
	context::context()
	{
		state_.open_libraries(sol::lib::base, sol::lib::package);
		state_.script("print('bark bark bark!')");
	}

	void context::run_frame()
	{
	}

	void context::notify(const event& /*e*/)
	{
	}
}
