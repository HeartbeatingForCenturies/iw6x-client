#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/scripting/entity.hpp"
#include "game/scripting/execution.hpp"

#include <utils/hook.hpp>

#include "command.hpp"

namespace logfile
{
	namespace
	{
		utils::hook::detour client_command_hook;

		void client_command_stub(const int clientNum, void* a2)
		{
			command::params_sv params = {};
			const auto cmd = params.get(0);

			if (cmd == "say"s || cmd == "say_team"s)
			{
				const auto player = scripting::call("getEntByNum", {clientNum}).as<scripting::entity>();

				auto msg = params.join(1);
				msg.erase(0, 1);

				scripting::notify(player, cmd, {msg});
			}

			client_command_hook.invoke<void>(clientNum, a2);
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_sp())
			{
				return;
			}

			client_command_hook.create(0x1403929B6, client_command_stub);
		}
	};
}

REGISTER_COMPONENT(logfile::component)
