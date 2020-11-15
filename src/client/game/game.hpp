#pragma once

#include "structs.hpp"
#include "launcher/launcher.hpp"

#define SELECT_VALUE(sp, mp) (game::environment::is_sp() ? (sp) : (mp))

#define SERVER_CD_KEY "IW6x-CD-Key"

namespace game
{
	namespace environment
	{
		launcher::mode get_mode();

		bool is_mp();
		bool is_sp();
		bool is_dedi();

		void set_mode(launcher::mode mode);
		void initialize();
	}

	template <typename T>
	class Function
	{
	public:
		Function(const size_t sp_address, const size_t mp_address)
			: sp_address_(sp_address)
			, mp_address_(mp_address)
		{
			
		}

		[[nodiscard]] size_t get() const
		{
			if(environment::is_sp())
			{
				return sp_address_;
			}
			
			return mp_address_;
		}

		operator T*() const
		{
			return reinterpret_cast<T*>(get());
		}

		template <typename... Args>
		auto operator()(Args... args)
		{
			return reinterpret_cast<T*>(get())(args...);
		}
		
	private:
		size_t sp_address_{0};
		size_t mp_address_{0};
	};

	static Function<void(int type, VariableUnion u)> AddRefToValue{0x1403D7740, 0x1404326E0};
	static Function<void(int type, VariableUnion u)> RemoveRefToValue{0x1403D90F0, 0x1404340C0};

	static Function<void()> Sys_ShowConsole{0x14043E650, 0x140503130};

	static Function<void ()> Com_Frame_Try_Block_Function{0x1403BC980, 0x1404131A0};
	static Function<const char* (char const**)> Com_Parse{0, 0x1404F50E0};
	static Function<void (errorParm code, const char* message, ...)> Com_Error{0x1403BBFF0, 0x140412740};
	static Function<void ()> Com_Quit{0x1403BDDD0, 0x140414920};
	static Function<CodPlayMode ()> Com_GetCurrentCoDPlayMode{0, 0x1404f6140};
	static Function<void (float, float, int)> Com_SetSlowMotion{0, 0x1404158C0};

	static Function<void (const char* message)> Conbuf_AppendText{0x14043DDE0, 0x1405028C0};

	static Function<void (int localClientNum, const char* text)> Cbuf_AddText{0x1403B3050, 0x1403F6B50};
	static Function<void (int localClientNum, int controllerIndex, const char* buffer,
								 void (int, int, const char*))> Cbuf_ExecuteBufferInternal{0x1403B3160, 0x1403F6C60};

	static Function<bool ()> CL_IsCgameInitialized{0x140234DA0, 0x1402B9A70};

	static Function<void (int localClientNum, const char* message)> CG_GameMessage{0x1401F2E20, 0x140271320};

	static Function<void (const char* cmdName, void (), cmd_function_s* allocedCmd)> Cmd_AddCommandInternal{0x1403B3570, 0x1403F7070};
	static Function<void (int localClientNum, int controllerIndex, const char* text)> Cmd_ExecuteSingleCommand{0x1403B3B10, 0x1403F7680};

	static Function<dvar_t* (const char* name)> Dvar_FindVar{0x140429E70, 0x1404ECB60};
	static Function<void (char* buffer, int index)> Dvar_GetCombinedString{0x1403BFD80, 0x140416B30};
	static Function<const char* (const char* dvarName, const char* defaultValue)> Dvar_GetVariantStringWithDefault{0x14042A240, 0x1404ECF90};
	static Function<dvar_t* (const char* dvarName, bool value, unsigned int flags,  const char* description)> Dvar_RegisterBool{0x14042AF10, 0x1404EDD60};
	static Function<dvar_t* (const char* dvarName, const char** valueList, int defaultIndex, unsigned int flags, const char* description)> Dvar_RegisterEnum{0x14042B220, 0x1404EE070};
	static Function<dvar_t* (const char* dvarName, float value, float min, float max, unsigned int flags, const char* description)> Dvar_RegisterFloat{0x14042B330, 0x1404EE180};
	static Function<dvar_t* (const char* dvarName, int value, int min, int max, unsigned int flags, const char* desc)> Dvar_RegisterInt{0x14042B420, 0x1404EE270};
	static Function<dvar_t* (const char* dvarName, const char* value, unsigned int flags, const char* description)> Dvar_RegisterString{0x14042B7A0, 0x1404EE660};
	static Function<dvar_t* (const char* dvarName, float x, float y, float min, float max, unsigned int flags, const char* description)> Dvar_RegisterVec2{0x14042B880, 0x1404EE740};
	static Function<dvar_t* (const char* dvarName, float x, float y, float z, float w, float min, float max, unsigned int flags, const char* description)> Dvar_RegisterVec4{0x14042BC10, 0x1404EEA50};
	static Function<void (dvar_t* dvar, DvarSetSource source)> Dvar_Reset{0x14042C150, 0x1404EF020};
	static Function<void (dvar_t* dvar, bool value)> Dvar_SetBool{0x14042C370, 0x1404EF1A0};
	static Function<void (const char* dvar, const char* buffer)> Dvar_SetCommand{0x14042C8E0, 0x1404EF790};
	static Function<void (dvar_t* dvar, const char* string)> Dvar_SetString{0x14042D6E0, 0x1404F08E0};
	static Function<void ()> Dvar_Sort{0x14042DEF0, 0x1404F1210};
	static Function<const char* (dvar_t* dvar, dvar_value value)> Dvar_ValueToString{0x14042E710, 0x1404F1A30};

	static Function<long long (const char* qpath, char** buffer)> FS_ReadFile{0x14041D0B0, 0x1404DE900};
	static Function<void (void* buffer)> FS_FreeFile{0x14041D0A0, 0x1404DE8F0};

	static Function<int (playerState_s* ps, Weapon weapon, int dualWield, int startInAltMode, int usedBefore)> G_GivePlayerWeapon{0x140359E20, 0x1403DA5E0};
	static Function<Weapon(const char* name)> G_GetWeaponForName{0x140359890, 0x1403DA060};
	static Function<void (playerState_s* ps, Weapon weapon, int hadWeapon)> G_InitializeAmmo{0x140311F00, 0x14039AEA0};
	static Function<unsigned int (const char* name, /*ConfigString*/ unsigned int start, unsigned int max, int create, const char* errormsg)> G_FindConfigstringIndex{0x0, 0x140161F90};
	static Function<int (int server_time)> G_RunFrame{0x0, 0x1403A05E0};

	static Function<game_hudelem_s* (int clientNum, int teamNum)> HudElem_Alloc{0x0, 0x1403997E0};

	static Function<char* (char* string)> I_CleanStr{0x140432460, 0x1404F63C0};

	static Function<const char* (int, int, int)> Key_KeynumToString{0x14023D9A0, 0x1402C40E0};

	static Function<unsigned int (int)> Live_SyncOnlineDataFlags{0, 0x1405ABF70};

	static Function<void (int clientNum, const char* menu, int a3, int a4, unsigned int a5)> LUI_OpenMenu{0x0, 0x1404B3610};

	static Function<bool (int clientNum, const char* menu)> Menu_IsMenuOpenAndVisible{0x0, 0x1404B38A0};

	static Function<Material* (const char* material)> Material_RegisterHandle{0x140523D90, 0x1405F0E20};

	static Function<void (netsrc_t, netadr_s*, const char*)> NET_OutOfBandPrint{0, 0x14041D5C0};
	static Function<bool (const char* s, game::netadr_s* a)> NET_StringToAdr{0, 0x14041D870};

	static Function<void (float x, float y, float width, float height, float s0, float t0, float s1, float t1, float* color, Material* material)> R_AddCmdDrawStretchPic{0x140234460, 0x140600BE0};
	static Function<void (const char*, int, Font_s*, float, float, float, float, float, float*, int)> R_AddCmdDrawText{0x140533E40, 0x140601070};
	static Function<void (const char*, int, Font_s*, float, float, float, float, float, const float*, int, int, char)> R_AddCmdDrawTextWithCursor{0x140534170, 0x1406013A0};
	static Function<Font_s* (const char* font)> R_RegisterFont{0x1405130B0, 0x1405DFAC0};
	static Function<int (const char* text, int maxChars, Font_s* font)> R_TextWidth{0x140513390, 0x1405DFDB0};

	static Function<ScreenPlacement* ()> ScrPlace_GetViewPlacement{0x14024D150, 0x1402F6D40};

	static Function<float (int index)> Scr_GetFloat{0, 0x140438D60};
	static Function<int ()> Scr_GetNumParam{0, 0x140438EC0};

	static Function<const char* (const char*)> SEH_StringEd_GetString{0x0, 0x1404A5F60};

	static Function<const char* (scr_string_t stringValue)> SL_ConvertToString{0x1403D6870, 0x1404317F0};

	static Function<void (int, int, const char*)> SV_GameSendServerCommand{0x140490F40, 0x1404758C0};
	static Function<bool ()> SV_Loaded{0x140491820, 0x1404770C0};
	static Function<void (int localClientNum, const char* map, bool mapIsPreloaded)> SV_StartMap{0, 0x140470170};
	static Function<void (int localClientNum, const char* map, bool mapIsPreloaded, bool migrate)> SV_StartMapForParty{0, 0x1404702F0};
	static Function<mp::gentity_s* (const char*, unsigned int, unsigned int, unsigned int)> SV_AddBot{0, 0x140470920};
	static Function<bool (int clientNum)> SV_BotIsBot{0, 0x140461340};
	static Function<void (mp::client_t*, const char*, int)> SV_ExecuteClientCommand{0, 0x140472430};
	static Function<void ()> SV_FastRestart{0x14048B890, 0x14046F440};
	static Function<playerState_s* (int num)> SV_GetPlayerstateForClientNum{0x140490F80, 0x140475A10};
	static Function<const char* (int clientNum)> SV_GetGuid{0, 0x140475990};
	static Function<void (int clientNum, const char* reason)> SV_KickClientNum{0, 0x14046F730};
	static Function<void (int index, const char* string)> SV_SetConfigstring{0, 0x140477450};
	static Function<void (mp::gentity_s*)> SV_SpawnTestClient{0, 0x1404740A0};

	static Function<bool ()> Sys_IsDatabaseReady2{0x1403C2D40, 0x140423920};
	static Function<int ()> Sys_Milliseconds{0x14043D2A0, 0x140501CA0};
	static Function<bool (int, void const*, const netadr_s*)> Sys_SendPacket{0x14043D000, 0x140501A00};

	static Function<void ()> SwitchToCoreMode{0, 0x1401FA4A0};
	static Function<void ()> SwitchToAliensMode{0, 0x1401FA4D0};
	static Function<void ()> SwitchToSquadVsSquadMode{0, 0x1401FA500};

	static Function<const char* (const char*)> UI_LocalizeMapname{0, 0x1404B96D0};
	static Function<const char* (const char*)> UI_LocalizeGametype{0, 0x1404B90F0};

	static Function<DWOnlineStatus (int)> dwGetLogOnStatus{0, 0x140589490};

	extern int* keyCatchers;

	extern CmdArgs* cmd_args;
	extern cmd_function_s** cmd_functions;

	extern int* dvarCount;
	extern dvar_t** sortedDvars;

	extern PlayerKeyState* playerKeys;

	extern SOCKET* query_socket;

	extern const char** command_whitelist;

	namespace sp
	{
		extern gentity_s* g_entities;
	}

	namespace mp
	{
		extern cg_s* cgArray;

		extern gentity_s* g_entities;

		extern int* svs_numclients;
		extern client_t* svs_clients;

		extern std::uint32_t* sv_serverId_value;

		extern int* gameTime;
		extern int* serverTime;
	}

	int Cmd_Argc();
	const char* Cmd_Argv(int index);

	int SV_Cmd_Argc();
	const char* SV_Cmd_Argv(int index);
}
