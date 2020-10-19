#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "game/game.hpp"
#include "game/dvars.hpp"

#include "utils/hook.hpp"

namespace gameplay
{
	namespace
	{
		const auto g_gravity_stub = utils::hook::assemble([](utils::hook::assembler& a)
		{
			a.push(rax);

			a.mov(rax, qword_ptr(reinterpret_cast<int64_t>(&dvars::g_gravity)));
			a.mov(rax, dword_ptr(rax, 0x10));
			a.mov(dword_ptr(rbx, 0x5C), eax);
			a.mov(eax, ptr(rbx, 0x33E8));
			a.mov(ptr(rbx, 0x25C), eax);

			a.pop(rax);

			a.jmp(0x1403828D5);
		});

		const auto g_speed_stub = utils::hook::assemble([](utils::hook::assembler& a)
		{
			a.push(rax);

			a.mov(rax, qword_ptr(reinterpret_cast<int64_t>(&dvars::g_speed)));
			a.mov(rax, dword_ptr(rax, 0x10));
			a.mov(dword_ptr(rdi, 0x60), eax);

			a.pop(rax);

			a.mov(eax, ptr(rdi, 0xEA4));
			a.add(eax, ptr(rdi, 0xEA0));

			a.jmp(0x140383796);
		});

		const auto pm_bouncing_stub_sp = utils::hook::assemble([](utils::hook::assembler& a)
		{
			const auto no_bounce = a.newLabel();
			const auto loc_14046ED26 = a.newLabel();

			a.push(rax);

			a.mov(rax, qword_ptr(reinterpret_cast<int64_t>(&dvars::pm_bouncing)));
			a.mov(al, byte_ptr(rax, 0x10));
			a.cmp(ptr(rbp, -0x40), al);

			a.pop(rax);
			a.jz(no_bounce);
			a.jmp(0x14046EC7E);

			a.bind(no_bounce);
			a.cmp(ptr(rbp, -0x80), r13d);
			a.jnz(loc_14046ED26);
			a.jmp(0x14046EC6C);

			a.bind(loc_14046ED26);
			a.jmp(0x14046ED26);
		});

		const auto pm_bouncing_stub_mp = utils::hook::assemble([](utils::hook::assembler& a)
		{
			const auto no_bounce = a.newLabel();
			const auto loc_140228FB8 = a.newLabel();

			a.push(rax);

			a.mov(rax, qword_ptr(reinterpret_cast<int64_t>(&dvars::pm_bouncing)));
			a.mov(al, byte_ptr(rax, 0x10));
			a.cmp(byte_ptr(rbp, -0x38), al);

			a.pop(rax);
			a.jz(no_bounce);
			a.jmp(0x140229019);

			a.bind(no_bounce);
			a.cmp(dword_ptr(rbp, -0x70), 0);
			a.jnz(loc_140228FB8);
			a.jmp(0x14022900B);

			a.bind(loc_140228FB8);
			a.jmp(0x140228FB8);
		});
	}

	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			// Implement bouncing dvar
			if (game::environment::is_sp())
			{
				utils::hook::nop(0x14046EC5C, 16);
			}

			utils::hook::jump(
				SELECT_VALUE(0x14046EC5C, 0x140228FFF), SELECT_VALUE(pm_bouncing_stub_sp, pm_bouncing_stub_mp), true);
			dvars::pm_bouncing = game::Dvar_RegisterBool("pm_bouncing", false, 
				game::DvarFlags::DVAR_FLAG_SAVED | game::DvarFlags::DVAR_FLAG_REPLICATED, "Enable bouncing");

			if (game::environment::is_sp()) return;

			// Implement gravity dvar
			utils::hook::nop(0x1403828C8, 13);
			utils::hook::jump(0x1403828C8, g_gravity_stub, true);
			dvars::g_gravity = game::Dvar_RegisterInt("g_gravity", 800, 0, 1000, 0,
			                                          "Game gravity in inches per second squared");

			// Implement speed dvar
			utils::hook::nop(0x140383789, 13);
			utils::hook::jump(0x140383789, g_speed_stub, true);
			dvars::g_speed = game::Dvar_RegisterInt("g_speed", 190, 0, 999, 0, "Maximum player speed");
		}
	};
}

REGISTER_MODULE(gameplay::module)
