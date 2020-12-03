#include <std_include.hpp>
#include "context.hpp"

namespace scripting::lua
{
	context::context(const std::string& file)
	{
		this->state_.open_libraries(sol::lib::base, sol::lib::package);

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
