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
		utils::hook::detour g_shutdown_game_hook;

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

		void g_shutdown_game_stub(const int free_scripts)
		{
			lua::engine::stop();
			return g_shutdown_game_hook.invoke<void>(free_scripts);
		}
	}

	int32_t has_config_string_index(const unsigned int csIndex)
	{
		const auto* s_constantConfigStringTypes = reinterpret_cast<uint8_t*>(0x141721F80);
		return csIndex < 0xDC4 && s_constantConfigStringTypes[csIndex] < 0x18u;
	}

	int is_pre_main_stub()
	{
		return game::CL_IsCgameInitialized();
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			vm_notify_hook.create(SELECT_VALUE(0x1403E29C0, 0x14043D9B0), vm_notify_stub);
			// SP address is wrong, but should be ok
			scr_load_level_hook.create(SELECT_VALUE(0x14013D5D0, 0x1403C4E60), scr_load_level_stub);
			g_shutdown_game_hook.create(SELECT_VALUE(0x140318C10, 0x1403A0DF0), g_shutdown_game_stub);

			scheduler::loop([]()
			{
				lua::engine::run_frame();
			}, scheduler::pipeline::server);

			if(!game::environment::is_sp())
			{
				// Make some room for pre_main hook
				utils::hook::jump(0x1402084A0, has_config_string_index);

				// Allow precaching anytime
				utils::hook::jump(0x1402084A5, is_pre_main_stub);
				utils::hook::set<uint16_t>(0x1402084D0, 0xD3EB); // jump to 0x1402084A5
			}
		}
	};
}

REGISTER_COMPONENT(scripting::component)
