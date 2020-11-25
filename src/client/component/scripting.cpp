#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"
#include "utils/hook.hpp"

#include "game/scripting/entity.hpp"
#include "game/scripting/execution.hpp"
#include "game/scripting/event.hpp"

#include "scheduler.hpp"

namespace scripting
{
	namespace
	{
		void test_hud_elem(const event& e)
		{
			const auto& player = e.entity;
			const auto name = player.get<std::string>("name");

			const auto hudelem = call<entity>("newHudElem");
			hudelem.set("fontscale", 1);
			hudelem.set("alpha", 1);

			hudelem.call("setText", {"^1Hello ^2" + name + "^5!"});

			player.call("iclientprintlnbold", {"^1The heli is following you!"});
		}

		void test_heli(const event& e)
		{
			test_hud_elem(e);

			const auto& player = e.entity;
			auto origin = player.get<vector>("origin");
			const auto angles = player.get<vector>("angles");

			origin[2] += 1000;

			player.call("freezeControls", {false});

			const auto heli = call<entity>("spawnhelicopter", {
				                               player, origin, angles, "cobra_mp", "vehicle_battle_hind"
			                               });

			heli.call("setturningability", {1});
			//heli.call("setlookatent", {player});
			heli.call("setspeed", {40, 15, 5});
			heli.call("setcandamage", {false});

			// This is bad. Need good scheduling
			scheduler::loop([player, heli]()
			{
				auto origin = player.get<vector>("origin");

				origin[2] += 1000;

				heli.call("setvehgoalpos", {origin, 1});
			}, scheduler::pipeline::server, 5s);
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
					//test_heli(e);
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
