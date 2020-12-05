#pragma once

#include "../event.hpp"

#define SOL_ALL_SAFETIES_ON 1
#define SOL_PRINT_ERRORS 0
#include <sol/sol.hpp>

#include "scheduler.hpp"
#include "event_handler.hpp"

namespace scripting::lua
{
	class context
	{
	public:
		context(const std::string& file);
		~context();

		context(context&&) noexcept = delete;
		context& operator=(context&&) noexcept = delete;

		context(const context&) = delete;
		context& operator=(const context&) = delete;

		void run_frame();
		void notify(const event& e);

	private:
		sol::state state_{};
		scheduler scheduler_;
		event_handler event_handler_;
	};
}
