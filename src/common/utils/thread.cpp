#include "thread.hpp"
#include "string.hpp"

#include <gsl/gsl>

namespace utils::thread
{
	bool set_name(const HANDLE t, const std::string& name)
	{
		return SUCCEEDED(SetThreadDescription(t, string::convert(name).data()));
	}

	bool set_name(const DWORD id, const std::string& name)
	{
		auto* const t = OpenThread(THREAD_SET_LIMITED_INFORMATION, FALSE, id);
		if (!t) return false;

		const auto _ = gsl::finally([t]()
		{
			CloseHandle(t);
		});

		return set_name(t, name);
	}

	bool set_name(std::thread& t, const std::string& name)
	{
		return set_name(t.native_handle(), name);
	}

	bool set_name(const std::string& name)
	{
		return set_name(GetCurrentThread(), name);
	}
}
