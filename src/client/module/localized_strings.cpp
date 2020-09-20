#include <std_include.hpp>

#include "loader/module_loader.hpp"

#include "utils/hook.hpp"

namespace
{
	utils::hook::detour seh_string_ed_get_string_hook;

	const char* seh_string_ed_get_string(const char* pszReference)
	{
		if (!strcmp(pszReference, "PLATFORM_SYSTEM_LINK_TITLE"))
		{
			return "SERVER LIST";
		}

		if (!strcmp(pszReference, "LUA_MENU_STORE_CAPS"))
		{
			return "SERVER LIST";
		}

		if (!strcmp(pszReference, "LUA_MENU_STORE_DESC"))
		{
			return "Browse available servers.";
		}

		return seh_string_ed_get_string_hook.invoke<const char*>(pszReference);
	}
}

class localized_strings final : public module
{
public:
	void post_unpack() override
	{
		// Change some localized strings
		seh_string_ed_get_string_hook.create(0x1404A5F60, &seh_string_ed_get_string);
	}
};

REGISTER_MODULE(localized_strings);
