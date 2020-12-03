#pragma once

#include "../event.hpp"

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

namespace scripting::lua
{
	class context
	{
	public:
		context();
		~context() = default;

		context(context&&) noexcept = default;
		context& operator=(context&&) noexcept = default;

		context(const context&) = delete;
		context& operator=(const context&) = delete;

		void run_frame();
		void notify(const event& e);

	private:
		sol::state state_{};
	};
}
