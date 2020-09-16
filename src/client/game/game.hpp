#pragma once

#include "structs.hpp"
#include "launcher/launcher.hpp"

#define SELECT_VALUE(sp, mp) (game::is_sp() ? (sp) : (mp))

#define SERVER_CD_KEY "IW6x-CD-Key"

namespace game
{
	namespace native
	{
		typedef void (*Sys_ShowConsole_t)();
		extern Sys_ShowConsole_t Sys_ShowConsole;

		typedef void (*Conbuf_AppendText_t)(const char* message);
		extern Conbuf_AppendText_t Conbuf_AppendText;

		typedef void (*Cbuf_AddText_t)(int localClientNum, const char* text);
		extern Cbuf_AddText_t Cbuf_AddText;

		typedef void (*Cmd_AddCommandInternal_t)(const char* cmdName, void(*function)(), cmd_function_s* allocedCmd);
		extern Cmd_AddCommandInternal_t Cmd_AddCommandInternal;

		typedef void (*Cmd_ExecuteSingleCommand_t)(int localClientNum, int controllerIndex, const char* text);
		extern Cmd_ExecuteSingleCommand_t Cmd_ExecuteSingleCommand;

		typedef dvar_t* (*Dvar_FindVar_t)(const char* name);
		extern Dvar_FindVar_t Dvar_FindVar;

		typedef dvar_t* (*Dvar_RegisterBool_t)(const char* dvarName, bool value, unsigned int flags, const char* description);
		extern Dvar_RegisterBool_t Dvar_RegisterBool;

		typedef dvar_t* (*Dvar_RegisterEnum_t)(const char* dvarName, const char** valueList, int defaultIndex, unsigned int flags, const char* description);
		extern Dvar_RegisterEnum_t Dvar_RegisterEnum;

		typedef dvar_t* (*Dvar_RegisterFloat_t)(const char* dvarName, float value, float min, float max, unsigned int flags, const char* description);
		extern Dvar_RegisterFloat_t Dvar_RegisterFloat;

		typedef dvar_t* (*Dvar_RegisterInt_t)(const char* dvarName, int value, int min, int max, unsigned int flags, const char* desc);
		extern Dvar_RegisterInt_t Dvar_RegisterInt;

		typedef dvar_t* (*Dvar_RegisterString_t)(const char* dvarName, const char* value, unsigned int flags, const char* description);
		extern Dvar_RegisterString_t Dvar_RegisterString;

		typedef dvar_t* (*Dvar_RegisterVec2_t)(const char* dvarName, float x, float y, float min, float max, unsigned int flags, const char* description);
		extern Dvar_RegisterVec2_t Dvar_RegisterVec2;

		typedef dvar_t* (*Dvar_RegisterVec4_t)(const char* dvarName, float x, float y, float z, float w, float min, float max, unsigned int flags, const char* description);
		extern Dvar_RegisterVec4_t Dvar_RegisterVec4;

		typedef void (*Dvar_Sort_t)();
		extern Dvar_Sort_t Dvar_Sort;

		typedef const char* (*Dvar_ValueToString_t)(dvar_t* dvar, dvar_value value);
		extern Dvar_ValueToString_t Dvar_ValueToString;

		typedef Material* (*Material_RegisterHandle_t)(const char* material);
		extern Material_RegisterHandle_t Material_RegisterHandle;

		typedef void (*R_AddCmdDrawStretchPic_t)(float x, float y, float width, float height, float s0, float t0, float s1, float t1, float* color, Material* material);
		extern R_AddCmdDrawStretchPic_t R_AddCmdDrawStretchPic;

		typedef void (*R_AddCmdDrawText_t)(const char*, int, Font_s*, float, float, float, float, float, float*, int);
		extern R_AddCmdDrawText_t R_AddCmdDrawText;

		typedef void (*R_AddCmdDrawTextWithCursor_t)(const char*, int, Font_s*, float, float, float, float, float, const float*, int, int, char);
		extern R_AddCmdDrawTextWithCursor_t R_AddCmdDrawTextWithCursor;

		typedef Font_s* (*R_RegisterFont_t)(const char* font);
		extern R_RegisterFont_t R_RegisterFont;

		typedef int (*R_TextWidth_t)(const char* text, int maxChars, Font_s* font);
		extern R_TextWidth_t R_TextWidth;

		typedef ScreenPlacement* (*ScrPlace_GetViewPlacement_t)();
		extern ScrPlace_GetViewPlacement_t ScrPlace_GetViewPlacement;

		extern clientUIActive_t* clientUIActives;

		extern CmdArgs* cmd_args;
		extern cmd_function_s** cmd_functions;

		extern int* dvarCount;
		extern dvar_t** sortedDvars;

		extern PlayerKeyState* playerKeys;

		int Cmd_Argc();
		const char* Cmd_Argv(int index);
	}

	bool is_mp();
	bool is_sp();
	bool is_dedi();

	void initialize(launcher::mode mode);
}
