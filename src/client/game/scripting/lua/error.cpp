#include <std_include.hpp>
#include "error.hpp"

namespace scripting::lua
{
	void handle_error(const sol::protected_function_result& result)
	{
		if (!result.valid())
		{
			printf("************** Script execution error **************\n");

			const sol::error err = result;
			printf("%s\n", err.what());

			printf("****************************************************\n");
		}
	}
}
