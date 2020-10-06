#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "localized_strings.hpp"
#include "utils/hook.hpp"
#include "utils/string.hpp"
#include "game/game.hpp"

namespace
{
	utils::hook::detour seh_string_ed_get_string_hook;

	std::unordered_map<std::string, std::string>& get_localized_overrides()
	{
		static std::unordered_map<std::string, std::string> overrides;
		return overrides;
	}

	std::mutex& get_synchronization_mutex()
	{
		static std::mutex mutex;
		return mutex;
	}

	const char* seh_string_ed_get_string(const char* pszReference)
	{
		std::lock_guard _(get_synchronization_mutex());

		auto& overrides = get_localized_overrides();
		const auto entry = overrides.find(pszReference);
		if (entry != overrides.end())
		{
			return utils::string::va("%s", entry->second.data());
		}

		return seh_string_ed_get_string_hook.invoke<const char*>(pszReference);
	}
}

void localized_strings::override(const std::string& key, const std::string& value)
{
	std::lock_guard _(get_synchronization_mutex());
	get_localized_overrides()[key] = value;
}

void localized_strings::post_unpack()
{
	if (game::environment::is_sp())
	{
		return;
	}

	// Change some localized strings
	seh_string_ed_get_string_hook.create(0x1404A5F60, &seh_string_ed_get_string);
}

REGISTER_MODULE(localized_strings);
