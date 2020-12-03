#include <std_include.hpp>
#include "engine.hpp"
#include "context.hpp"

namespace scripting::lua::engine
{
	namespace
	{
		std::vector<context>& get_scripts()
		{
			static std::vector<context> scripts{};
			return scripts;
		}
	}

	void start()
	{
	}

	void stop()
	{
		get_scripts().clear();
	}

	void notify(const event& e)
	{
		for (auto& script : get_scripts())
		{
			script.notify(e);
		}
	}

	void run_frame()
	{
		for (auto& script : get_scripts())
		{
			script.run_frame();
		}
	}
}
