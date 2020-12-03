#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"
#include <utils/hook.hpp>

#include "game/scripting/entity.hpp"
#include "game/scripting/event.hpp"
#include "game/scripting/lua/engine.hpp"

#include "scheduler.hpp"

namespace scripting
{
	namespace
	{
		utils::hook::detour vm_notify_hook;
		utils::hook::detour scr_load_level_hook;
		utils::hook::detour scr_shutdown_system_hook;

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

				lua::engine::notify(e);
			}

			vm_notify_hook.invoke<void>(notify_list_owner_id, string_value, top);
		}

		void scr_load_level_stub()
		{
			scr_load_level_hook.invoke<void>();
			lua::engine::start();
		}

		void scr_shutdown_system_stub(const char sys)
		{
			lua::engine::stop();
			return scr_shutdown_system_hook.invoke<void>(sys);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			vm_notify_hook.create(SELECT_VALUE(0x1403E29C0, 0x14043D9B0), vm_notify_stub);
			// SP address is wrong, but should be ok
			scr_load_level_hook.create(SELECT_VALUE(0x1403163D9, 0x1403C4E60), scr_load_level_stub);
			scr_shutdown_system_hook.create(SELECT_VALUE(0x1403DE990, 0x140439970), scr_shutdown_system_stub);

			scheduler::loop([]()
			{
				lua::engine::run_frame();
			}, scheduler::pipeline::server);
		}
	};
}

REGISTER_COMPONENT(scripting::component)
