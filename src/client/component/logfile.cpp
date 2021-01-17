#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "scheduler.hpp"

#include "game/scripting/entity.hpp"
#include "game/scripting/execution.hpp"

#include <utils/hook.hpp>

namespace logfile
{
	namespace
	{
		bool evaluate_say(char* text, game::mp::gentity_s* ent)
		{
			auto hidden = false;

			++text;

			if (text[0] == '/')
			{
				hidden = true;
				++text;
			}

			const std::string message = text;
			const auto client = ent->client->ps.clientNum;

			scheduler::once([message, client]()
			{
				const scripting::entity level{*game::levelEntityId};
				const auto player = scripting::call("getEntByNum", {client}).as<scripting::entity>();

				scripting::notify(level, "say", {player, message});
				scripting::notify(player, "say", {message});
			}, scheduler::pipeline::server);

			return hidden;
		}
	}

	const auto say_stub = utils::hook::assemble([](utils::hook::assembler& a)
	{
		const auto hidden = a.newLabel();

		a.pushad64();
		a.mov(rcx, rbx);
		a.mov(rdx, rdi);

		a.call_aligned(evaluate_say);

		a.cmp(al, 0);
		a.jne(hidden);

		a.popad64();
		a.lea(rcx, dword_ptr(rsp, 0x80));
		a.mov(r8d, 0x96);
		a.jmp(0x140392A98);

		a.bind(hidden);
		a.popad64();
		a.jmp(0x140392B04);
	});

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_sp())
			{
				return;
			}

			utils::hook::jump(0x140392A8A, say_stub, true);
		}
	};
}

REGISTER_COMPONENT(logfile::component)
