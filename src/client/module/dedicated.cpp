#include <std_include.hpp>

#include "scheduler.hpp"
#include "loader/module_loader.hpp"
#include "utils/nt.hpp"
#include "utils/hook.hpp"
#include "game/game.hpp"

namespace
{
	void init_dedicated_server()
	{
		static bool initialized = false;
		if (initialized) return;
		initialized = true;

		static const char* fastfiles[] =
		{
			"code_post_gfx_mp",
			"ui_mp",
			"code_nvidia_mp",
			"common_mp",
			"mp_character_room",
			"mp_character_room_heads",
			"mp_character_room_bodies_updated",
			"mp_character_room_dlc_updated",
			"techsets_common_core_mp",
			"common_core_mp",
			"common_core_dlc_updated_mp",
			"techsets_common_alien_mp",
			"common_alien_mp",
			"common_alien_dlc_updated_mp",
			0
		};

		// load fastfiles
		std::memcpy(reinterpret_cast<void*>(0x1480B1E40), &fastfiles, sizeof(fastfiles));

		// R_LoadGraphicsAssets
		((void(*)())0x1405E6F80)();
	}
}

class dedicated final : public module
{
public:
	void post_unpack() override
	{
		if (!game::environment::is_dedi())
		{
			return;
		}

		//utils::hook::set<uint8_t>(0x1402C89A0, 0xC3);											// R_Init caller
		utils::hook::jump(0x1402C89A0, init_dedicated_server);

		utils::hook::set<uint8_t>(0x1402E5830, 0xC3); // disable self-registration
		utils::hook::set<std::uint8_t>(0x1402C7935, 5);											// make CL_Frame do client packets, even for game state 9
		//utils::hook::set<std::uint8_t>(0x5C6F90, 0xC3);											// init sound system (1)
		utils::hook::set<uint8_t>(0x140602380, 0xC3); // start render thread
		utils::hook::set<uint8_t>(0x140658580, 0xC3); // init sound system (2)
		//utils::hook::set<std::uint8_t>(0x49BC10, 0xC3);											// Com_Frame audio processor?
		//utils::hook::set<std::uint8_t>(0x4978F0, 0xC3); 										// called from Com_Frame, seems to do renderer stuff
		//utils::hook::set<std::uint8_t>(0x48FF30, 0xC3);											// CL_CheckForResend, which tries to connect to the local server constantly
		//utils::hook::set<std::uint8_t>(0x5CC160, 0xC3); 										// function detecting video card, causes Direct3DCreate9 to be called
		utils::hook::set<uint8_t>(0x1405DAE1F, 0); // r_loadForRenderer default to 0
		utils::hook::set<uint8_t>(0x1404FFCE2, 0xC3); // recommended settings check - TODO: Check hook
		utils::hook::set<uint8_t>(0x140503420, 0xC3); // some mixer-related function called on shutdown
		utils::hook::set<uint8_t>(0x1404BEC10, 0xC3); // dont load ui gametype stuff
		//utils::hook::set<std::uint8_t>(0x611690, 0xC3); 										// some unknown function that seems to fail
		//utils::hook::nop(0x572238, 6); 															// unknown check in SV_ExecuteClientMessage
		//utils::hook::nop(0x5751DF, 2);															// don't spawn a DemonWare session for the dedicated server
		//utils::hook::set<std::uint8_t>(0x5751E9, 0xEB);											// ^
		//utils::hook::set<std::uint8_t>(0x572ED3, 0xEB);											// allow first slot to be occupied
		//utils::hook::nop(0x48FEF9, 2);															// properly shut down dedicated servers
		//utils::hook::nop(0x48FED3, 2);															// ^
		//utils::hook::nop(0x48FF14, 5);															// don't shutdown renderer
		//utils::hook::set<std::uint8_t>(0x575266, 0xEB);											// shutdown existing stuff before loading new map

		utils::hook::set<uint8_t>(0x1404FFCF0, 0xC3); // cpu detection stuff

		scheduler::loop([]()
		{
			MSG msg;
			while (PeekMessageA(&msg, nullptr, NULL, NULL, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}, scheduler::pipeline::main);
	}
};

REGISTER_MODULE(dedicated)
