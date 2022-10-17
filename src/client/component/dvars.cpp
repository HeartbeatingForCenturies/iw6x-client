#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "game/game.hpp"

#include "dvars.hpp"

#include <utils/hook.hpp>

namespace dvars
{
	struct dvar_base
	{
		unsigned int flags{};
	};

	struct dvar_bool : dvar_base
	{
		bool value{};
	};

	utils::hook::detour dvar_register_bool_hook;

	template <typename T>
	T* find_dvar(std::unordered_map<std::string, T>& map, const std::string& name)
	{
		auto i = map.find(name);
		if (i != map.end())
		{
			return &i->second;
		}

		return nullptr;
	}

	namespace override
	{
		static std::unordered_map<std::string, dvar_bool> register_bool_overrides;

		void Dvar_RegisterBool(const std::string& name, const bool value, const unsigned int flags)
		{
			dvar_bool values;
			values.value = value;
			values.flags = flags;
			register_bool_overrides[name] = values;
		}
	}

	const game::dvar_t* dvar_register_bool(const char* name, bool value, unsigned int flags, const char* description)
	{
		const auto* var = find_dvar(override::register_bool_overrides, name);
		if (var)
		{
			value = var->value;
			flags = var->flags;
		}

		return dvar_register_bool_hook.invoke<const game::dvar_t*>(name, value, flags, description);
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			dvar_register_bool_hook.create(game::Dvar_RegisterBool, &dvar_register_bool);
		}

		void pre_destroy() override
		{
			dvar_register_bool_hook.clear();
		}
	};
}

REGISTER_COMPONENT(dvars::component)
