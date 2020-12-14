#pragma once

namespace scripting::lua
{
	void handle_error(const std::exception& e);

	template <typename F>
	auto safe(F&& f) -> decltype(f())
	{
		try
		{
			return f();
		}
		catch (std::exception& e)
		{
			handle_error(e);
		}

		if constexpr (!std::is_same<decltype(f()), void>::value)
		{
			return decltype(f()){};
		}
	}
}
