#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "scheduler.hpp"
#include "utils/hook.hpp"
#include "game/game.hpp"

namespace arxan
{
	namespace
	{
		volatile bool allow_lobby_pump = true;
		volatile bool allow_net_pump = true;
		volatile bool allow_logon_status = true;

		utils::hook::detour lobby_pump_hook;
		utils::hook::detour net_pump_hook;

		game::DWOnlineStatus get_logon_status_stub(const int controller_index)
		{
			static volatile auto is_connected = false;
			if (is_connected && !allow_logon_status)
			{
				return game::DW_LIVE_CONNECTED;
			}


			const auto result = reinterpret_cast<game::DWOnlineStatus(*)(int)>(0x1405894C0)(controller_index);
			is_connected = result == game::DW_LIVE_CONNECTED;
			allow_logon_status = false;

			return result;
		}

		void lobby_pump_stub(const int controller_index)
		{
			if (allow_lobby_pump || get_logon_status_stub(0) != game::DW_LIVE_CONNECTED)
			{
				allow_lobby_pump = false;
				lobby_pump_hook.invoke<void*>(controller_index);
			}
		}

		void net_pump_stub(const int controller_index)
		{
			if (allow_net_pump || get_logon_status_stub(0) != game::DW_LIVE_CONNECTED)
			{
				allow_net_pump = false;
				net_pump_hook.invoke<void*>(controller_index);
			}
		}
	}

	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			// cba to implement sp, not sure if it's even needed
			if (game::environment::is_sp()) return;

			scheduler::loop([]
			{
				allow_lobby_pump = true;
				allow_net_pump = true;
				allow_logon_status = true;
			}, scheduler::pipeline::async, 300ms);

			// Some arxan exception stuff to obfuscate functions
			// This is causing lags
			// We will have to properly patch that some day
			utils::hook::jump(0x140589480, get_logon_status_stub);

			lobby_pump_hook.create(0x140591850, lobby_pump_stub);
			net_pump_hook.create(0x140558C20, net_pump_stub);
		}
	};
}

REGISTER_MODULE(arxan::module)
