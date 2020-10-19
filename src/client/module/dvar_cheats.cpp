#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "utils/hook.hpp"
#include "game/game.hpp"

#include "game_console.hpp"
#include "scheduler.hpp"

namespace dvar_cheats
{
	void apply_sv_cheats(const game::dvar_t* dvar, const game::DvarSetSource source, game::dvar_value* value)
	{
		if (dvar && dvar->name == "sv_cheats"s)
		{
			// if dedi, do not allow internal to change value so servers can allow cheats if they want to
			if (game::environment::is_dedi() && source == game::DvarSetSource::DVAR_SOURCE_INTERNAL)
			{
				value->enabled = dvar->current.enabled;
			}

			// if sv_cheats was enabled and it changes to disabled, we need to reset all cheat dvars
			else if (dvar->current.enabled && !value->enabled)
			{
				for (auto i = 0; i < *game::dvarCount; ++i)
				{
					const auto var = game::sortedDvars[i];
					if (var && (var->flags & game::DvarFlags::DVAR_FLAG_CHEAT))
					{
						game::Dvar_Reset(var, game::DvarSetSource::DVAR_SOURCE_INTERNAL);
					}
				}
			}
		}
	}

	bool dvar_flag_checks(const game::dvar_t* dvar, const game::DvarSetSource source)
	{
		if ((dvar->flags & game::DvarFlags::DVAR_FLAG_WRITE))
		{
			game_console::print(1, "%s is write protected", dvar->name);
			return false;
		}

		if ((dvar->flags & game::DvarFlags::DVAR_FLAG_READ))
		{
			game_console::print(1, "%s is read only", dvar->name);
			return false;
		}

		// only check cheat/replicated values when the source is external
		if (source == game::DvarSetSource::DVAR_SOURCE_EXTERNAL)
		{
			const auto cl_ingame = game::Dvar_FindVar("cl_ingame");
			const auto sv_running = game::Dvar_FindVar("sv_running");

			if ((dvar->flags & game::DvarFlags::DVAR_FLAG_REPLICATED) && (cl_ingame && cl_ingame->current.enabled) && (sv_running && !sv_running->current.enabled))
			{
				game_console::print(1, "%s can only be changed by the server", dvar->name);
				return false;
			}

			const auto sv_cheats = game::Dvar_FindVar("sv_cheats");
			if ((dvar->flags & game::DvarFlags::DVAR_FLAG_CHEAT) && (sv_cheats && !sv_cheats->current.enabled))
			{
				game_console::print(1, "%s is cheat protected", dvar->name);
				return false;
			}
		}

		// pass all the flag checks, allow dvar to be changed
		return true;
	}

	const auto dvar_flag_checks_stub = utils::hook::assemble([](utils::hook::assembler& a)
	{
		const auto can_set_value = a.newLabel();
		const auto zero_source = a.newLabel();

		a.pushad64();
		a.mov(r8, rdi);
		a.mov(edx, esi);
		a.mov(rcx, rbx);
		a.call(apply_sv_cheats); //check if we are setting sv_cheats
		a.popad64();
		a.cmp(esi, 0);
		a.jz(zero_source); //if the SetSource is 0 (INTERNAL) ignore flag checks

		a.pushad64();
		a.mov(edx, esi); //source
		a.mov(rcx, rbx); //dvar
		a.call(dvar_flag_checks); //protect read/write/cheat/replicated dvars
		a.cmp(al, 1);
		a.jz(can_set_value);

		// if we get here, we are non-zero source and CANNOT set values
		a.popad64(); // if I do this before the jz it won't work. for some reason the popad64 is affecting the ZR flag
		a.jmp(0x1404F0FC5);

		// if we get here, we are non-zero source and CAN set values
		a.bind(can_set_value);
		a.popad64(); // if I do this before the jz it won't work. for some reason the popad64 is affecting the ZR flag
		a.jmp(0x1404F0D2E);
		
		// if we get here, we are zero source and ignore flags
		a.bind(zero_source);
		a.jmp(0x1404F0D74);
	});

	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_sp()) return;

			utils::hook::nop(0x1404F0D13, 4); // let our stub handle zero-source sets
			utils::hook::jump(0x1404F0D1A, dvar_flag_checks_stub, true); // check extra dvar flags when setting values
			
			scheduler::once([]()
			{
				game::Dvar_RegisterBool("sv_cheats", false, game::DvarFlags::DVAR_FLAG_REPLICATED, "Allow cheat commands and dvars on this server");
			}, scheduler::pipeline::main);
		}
	};
}

REGISTER_MODULE(dvar_cheats::module)
