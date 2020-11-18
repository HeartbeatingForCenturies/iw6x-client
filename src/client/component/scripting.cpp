#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"
#include "utils/hook.hpp"

#include "game/scripting/entity.hpp"
#include "game/scripting/execution.hpp"
#include "game/scripting/event.hpp"

namespace scripting
{
	namespace
	{
		void test_call(const event& e)
		{
			const auto& player = e.entity;
			const auto name = player.get<std::string>("name");

			const auto hudelem = call<entity>("newHudElem");
			hudelem.set("fontscale", 1);
			hudelem.set("alpha", 1);

			hudelem.call("setText", {"^1Hello ^2" + name + "^5!"});
		}

		utils::hook::detour vm_notify_hook;

		void vm_notify_stub(const unsigned int notify_list_owner_id, const game::scr_string_t string_value,
		                    game::VariableValue* top)
		{
			const auto* string = game::SL_ConvertToString(string_value);
			if (string)
			{
				event e;
				e.name = string;
				e.entity = notify_list_owner_id;

				for (auto* value = top; value->type != game::SCRIPT_END; --value)
				{
					e.arguments.emplace_back(*value);
				}

#ifdef DEV_BUILD
				if (e.name == "spawned_player")
				{
					test_call(e);
				}
#endif
			}

			vm_notify_hook.invoke<void>(notify_list_owner_id, string_value, top);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			vm_notify_hook.create(SELECT_VALUE(0x1403E29C0, 0x14043D9B0), vm_notify_stub);
		}
	};
}

REGISTER_COMPONENT(scripting::component)
