#include <std_include.hpp>
#include "game.hpp"

namespace game
{
	namespace native
	{
		Sys_ShowConsole_t Sys_ShowConsole;
		Conbuf_AppendText_t Conbuf_AppendText;

		Cbuf_AddText_t Cbuf_AddText;

		CG_GameMessage_t CG_GameMessage;

		CL_IsCgameInitialized_t CL_IsCgameInitialized;

		Cmd_AddCommandInternal_t Cmd_AddCommandInternal;
		Cmd_ExecuteSingleCommand_t Cmd_ExecuteSingleCommand;

		Dvar_FindVar_t Dvar_FindVar;
		Dvar_RegisterBool_t Dvar_RegisterBool;
		Dvar_RegisterEnum_t Dvar_RegisterEnum;
		Dvar_RegisterFloat_t Dvar_RegisterFloat;
		Dvar_RegisterInt_t Dvar_RegisterInt;
		Dvar_RegisterString_t Dvar_RegisterString;
		Dvar_RegisterVec2_t Dvar_RegisterVec2;
		Dvar_RegisterVec4_t Dvar_RegisterVec4;
		Dvar_Sort_t Dvar_Sort;
		Dvar_ValueToString_t Dvar_ValueToString;

		LUI_OpenMenu_t LUI_OpenMenu;

		Material_RegisterHandle_t Material_RegisterHandle;

		R_AddCmdDrawStretchPic_t R_AddCmdDrawStretchPic;
		R_AddCmdDrawText_t R_AddCmdDrawText;
		R_AddCmdDrawTextWithCursor_t R_AddCmdDrawTextWithCursor;
		R_RegisterFont_t R_RegisterFont;
		R_TextWidth_t R_TextWidth;

		ScrPlace_GetViewPlacement_t ScrPlace_GetViewPlacement;

		SEH_StringEd_GetString_t SEH_StringEd_GetString;

		SV_GameSendServerCommand_t SV_GameSendServerCommand;
		SV_Loaded_t SV_Loaded;

		Sys_SendPacket_t Sys_SendPacket;

		int* keyCatchers;

		CmdArgs* cmd_args;
		CmdArgs* sv_cmd_args;
		cmd_function_s** cmd_functions;

		int* dvarCount;
		dvar_t** sortedDvars;

		PlayerKeyState* playerKeys;

		SOCKET* query_socket;

		namespace sp
		{
			gentity_s* g_entities;
		}

		namespace mp
		{
			gentity_s* g_entities;
		}

		int Cmd_Argc()
		{
			return native::cmd_args->argc[native::cmd_args->nesting];
		}

		const char* Cmd_Argv(int index)
		{
			return native::cmd_args->argv[native::cmd_args->nesting][index];
		}

		int SV_Cmd_Argc()
		{
			return native::sv_cmd_args->argc[native::sv_cmd_args->nesting];
		}

		const char* SV_Cmd_Argv(int index)
		{
			return native::sv_cmd_args->argv[native::sv_cmd_args->nesting][index];
		}
	}

	launcher::mode mode = launcher::mode::none;

	launcher::mode get_mode()
	{
		if (mode == launcher::mode::none)
		{
			throw std::runtime_error("Launcher mode not valid. Something must be wrong.");
		}

		return mode;
	}

	bool is_mp()
	{
		return get_mode() == launcher::mode::multiplayer;
	}

	bool is_sp()
	{
		return get_mode() == launcher::mode::singleplayer;
	}

	bool is_dedi()
	{
		return get_mode() == launcher::mode::server;
	}

	void initialize(const launcher::mode _mode)
	{
		mode = _mode;

		native::Sys_ShowConsole = native::Sys_ShowConsole_t(SELECT_VALUE(0x14043E650, 0x140503130));
		native::Conbuf_AppendText = native::Conbuf_AppendText_t(SELECT_VALUE(0x14043DDE0, 0x1405028C0));

		native::Cbuf_AddText = native::Cbuf_AddText_t(SELECT_VALUE(0x1403B3050, 0x1403F6B50));

		native::CG_GameMessage = native::CG_GameMessage_t(SELECT_VALUE(0x1401F2E20, 0x140271320));

		native::CL_IsCgameInitialized = native::CL_IsCgameInitialized_t(SELECT_VALUE(0x140234DA0, 0x1402B9A70));

		native::Cmd_AddCommandInternal = native::Cmd_AddCommandInternal_t(SELECT_VALUE(0x1403B3570, 0x1403F7070));
		native::Cmd_ExecuteSingleCommand = native::Cmd_ExecuteSingleCommand_t(SELECT_VALUE(0x1403B3B10, 0x1403F7680));

		native::Dvar_FindVar = native::Dvar_FindVar_t(SELECT_VALUE(0x140429E70, 0x1404ECB60));
		native::Dvar_RegisterBool = native::Dvar_RegisterBool_t(SELECT_VALUE(0x14042AF10, 0x1404EDD60));
		native::Dvar_RegisterEnum = native::Dvar_RegisterEnum_t(SELECT_VALUE(0x14042B220, 0x1404EE070));
		native::Dvar_RegisterFloat = native::Dvar_RegisterFloat_t(SELECT_VALUE(0x14042B330, 0x1404EE180));
		native::Dvar_RegisterInt = native::Dvar_RegisterInt_t(SELECT_VALUE(0x14042B420, 0x1404EE270));
		native::Dvar_RegisterString = native::Dvar_RegisterString_t(SELECT_VALUE(0x14042B7A0, 0x1404EE660));
		native::Dvar_RegisterVec2 = native::Dvar_RegisterVec2_t(SELECT_VALUE(0x14042B880, 0x1404EE740));
		native::Dvar_RegisterVec4 = native::Dvar_RegisterVec4_t(SELECT_VALUE(0x14042BC10, 0x1404EEA50));
		native::Dvar_Sort = native::Dvar_Sort_t(SELECT_VALUE(0x14042DEF0, 0x1404F1210));
		native::Dvar_ValueToString = native::Dvar_ValueToString_t(SELECT_VALUE(0x14042E710, 0x1404F1A30));

		native::LUI_OpenMenu = native::LUI_OpenMenu_t(SELECT_VALUE(0x0, 0x1404B3610));

		native::Material_RegisterHandle = native::Material_RegisterHandle_t(SELECT_VALUE(0x140523D90, 0x1405F0E20));

		native::R_AddCmdDrawStretchPic = native::R_AddCmdDrawStretchPic_t(SELECT_VALUE(0x140234460, 0x140600BE0));
		native::R_AddCmdDrawText = native::R_AddCmdDrawText_t(SELECT_VALUE(0x140533E40, 0x140601070));
		native::R_AddCmdDrawTextWithCursor = native::R_AddCmdDrawTextWithCursor_t(SELECT_VALUE(0x140534170, 0x1406013A0));
		native::R_RegisterFont = native::R_RegisterFont_t(SELECT_VALUE(0x1405130B0, 0x1405DFAC0));
		native::R_TextWidth = native::R_TextWidth_t(SELECT_VALUE(0x140513390, 0x1405DFDB0));

		native::ScrPlace_GetViewPlacement = native::ScrPlace_GetViewPlacement_t(SELECT_VALUE(0x14024D150, 0x1402F6D40));

		native::SEH_StringEd_GetString = native::SEH_StringEd_GetString_t(SELECT_VALUE(0x0, 0x1404A5F60));

		native::SV_GameSendServerCommand = native::SV_GameSendServerCommand_t(SELECT_VALUE(0x140490F40, 0x1404758C0));
		native::SV_Loaded = native::SV_Loaded_t(SELECT_VALUE(0x140491820, 0x1404770C0));
		
		native::Sys_SendPacket = native::Sys_SendPacket_t(SELECT_VALUE(0x14043D000, 0x140501A00));

		native::keyCatchers = reinterpret_cast<int*>(SELECT_VALUE(0x1417CF6E0, 0x1419E1ADC));

		native::cmd_args = reinterpret_cast<native::CmdArgs*>(SELECT_VALUE(0x144CE7F70, 0x144518480));
		native::sv_cmd_args = reinterpret_cast<native::CmdArgs*>(SELECT_VALUE(0x144CE8020, 0x144518530));
		native::cmd_functions = reinterpret_cast<native::cmd_function_s**>(SELECT_VALUE(0x144CE80C8, 0x1445185D8));

		native::dvarCount = reinterpret_cast<int*>(SELECT_VALUE(0x1458CBA3C, 0x1478EADF4));
		native::sortedDvars = reinterpret_cast<native::dvar_t**>(SELECT_VALUE(0x1458CBA60, 0x1478EAE10));

		native::playerKeys = reinterpret_cast<native::PlayerKeyState*>(SELECT_VALUE(0x14164138C, 0x1419DEABC));
		
		native::query_socket = reinterpret_cast<SOCKET*>(SELECT_VALUE(0, 0x147AD1A78));

		if (is_sp())
		{
			native::sp::g_entities = reinterpret_cast<native::sp::gentity_s*>(0x143C91600);
		}
		else if (is_mp())
		{
			native::mp::g_entities = reinterpret_cast<native::mp::gentity_s*>(0x14427A0E0);
		}
	}
}
