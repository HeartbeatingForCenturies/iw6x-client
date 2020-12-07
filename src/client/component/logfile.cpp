#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/scripting/entity.hpp"
#include "game/scripting/execution.hpp"

#include "command.hpp"

namespace logfile
{
	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			command::add_sv("say", [](const int clientNum, const command::params_sv& params)
			{
				const auto player = scripting::call("getEntByNum", {clientNum}).as<scripting::entity>();

				auto msg = params.join(1);
				msg.erase(0, 1);

				scripting::notify(player, "say", {msg});
			});

			command::add_sv("say_team", [](const int clientNum, const command::params_sv& params)
			{
				const auto player = scripting::call("getEntByNum", {clientNum}).as<scripting::entity>();

				auto msg = params.join(1);
				msg.erase(0, 1);

				scripting::notify(player, "say_team", {msg});
			});
		}
	};
}

REGISTER_COMPONENT(logfile::component)
