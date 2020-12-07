#include <std_include.hpp>
#include "error.hpp"

namespace scripting::lua
{
	void handle_error(const std::exception& e)
	{
		printf("************** Script execution error **************\n");
		printf("%s\n", e.what());
		printf("****************************************************\n");
	}
}
