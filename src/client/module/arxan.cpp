#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "scheduler.hpp"
#include "utils/hook.hpp"
#include "game/game.hpp"

namespace arxan
{
	namespace
	{
		typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFORMATION
		{
			BOOLEAN Inherit;
			BOOLEAN ProtectFromClose;
		} OBJECT_HANDLE_ATTRIBUTE_INFORMATION;

		utils::hook::detour nt_close_hook;
		utils::hook::detour nt_query_information_process;

		NTSTATUS WINAPI nt_query_information_process_stub(const HANDLE handle, const PROCESSINFOCLASS info_class,
		                                                  PVOID info,
		                                                  const ULONG info_length, const PULONG ret_length)
		{
			auto* orig = static_cast<decltype(NtQueryInformationProcess)*>(nt_query_information_process.get_original());
			const auto status = orig(handle, info_class, info, info_length, ret_length);

			if (NT_SUCCESS(status))
			{
				if (info_class == ProcessBasicInformation)
				{
					static DWORD explorerPid = 0;
					if (!explorerPid)
					{
						auto* const shell_window = GetShellWindow();
						GetWindowThreadProcessId(shell_window, &explorerPid);
					}

					static_cast<PPROCESS_BASIC_INFORMATION>(info)->Reserved3 = PVOID(DWORD64(explorerPid));
				}
				else if (info_class == 30) // ProcessDebugObjectHandle
				{
					*static_cast<HANDLE*>(info) = nullptr;

					return 0xC0000353;
				}
				else if (info_class == 7) // ProcessDebugPort
				{
					*static_cast<HANDLE*>(info) = nullptr;
				}
				else if (info_class == 31)
				{
					*static_cast<ULONG*>(info) = 1;
				}
			}

			return status;
		}

		NTSTATUS NTAPI nt_close_stub(const HANDLE handle)
		{
			char info[16];
			if (NtQueryObject(handle, OBJECT_INFORMATION_CLASS(4), &info, sizeof(OBJECT_HANDLE_ATTRIBUTE_INFORMATION),
			                  nullptr) >= 0)
			{
				auto* orig = static_cast<decltype(NtClose)*>(nt_close_hook.get_original());
				return orig(handle);
			}

			return STATUS_INVALID_HANDLE;
		}

		LONG WINAPI exception_filter(LPEXCEPTION_POINTERS info)
		{
			return (info->ExceptionRecord->ExceptionCode == STATUS_INVALID_HANDLE)
				       ? EXCEPTION_CONTINUE_EXECUTION
				       : EXCEPTION_CONTINUE_SEARCH;
		}

		void hide_being_debugged()
		{
			auto* const peb = PPEB(__readgsqword(0x60));
			peb->BeingDebugged = false;
			*PDWORD(LPSTR(peb) + 0xBC) &= ~0x70;
		}

		volatile bool allow_lobby_pump = true;
		utils::hook::detour lobby_pump_hook;

		void lobby_pump_stub(const int controller_index)
		{
			if (allow_lobby_pump)
			{
				allow_lobby_pump = false;
				lobby_pump_hook.invoke<void*>(controller_index);
			}
		}

		volatile bool allow_net_pump = true;
		utils::hook::detour net_pump_hook;

		void net_pump_stub(const int controller_index)
		{
			if (allow_net_pump)
			{
				allow_net_pump = false;
				net_pump_hook.invoke<void*>(controller_index);
			}
		}

		game::DWOnlineStatus get_logon_status_stub(const int controller_index)
		{
			static auto is_connected = false;
			if (is_connected)
			{
				return game::DW_LIVE_CONNECTED;
			}

			const auto result = reinterpret_cast<game::DWOnlineStatus(*)(int)>(0x1405894C0)(controller_index);
			if (result == game::DW_LIVE_CONNECTED)
			{
				is_connected = true;
				lobby_pump_hook.create(0x140591850, lobby_pump_stub);
				net_pump_hook.create(0x140558C20, net_pump_stub);
			}

			return result;
		}
	}

	class module final : public module_interface
	{
	public:
		void post_load() override
		{
			hide_being_debugged();
			scheduler::loop(hide_being_debugged, scheduler::pipeline::async);


			const utils::nt::module ntdll("ntdll.dll");
			nt_close_hook.create(ntdll.get_proc<void*>("NtClose"), nt_close_stub);
			nt_query_information_process.create(ntdll.get_proc<void*>("NtQueryInformationProcess"),
			                                    nt_query_information_process_stub);

			AddVectoredExceptionHandler(1, exception_filter);
		}

		/*void post_unpack() override
		{
			// cba to implement sp, not sure if it's even needed
			if (game::environment::is_sp()) return;

			scheduler::loop([]
			{
				allow_lobby_pump = true;
				allow_net_pump = true;
			}, scheduler::pipeline::async, 300ms);

			// Some arxan exception stuff to obfuscate functions
			// This is causing lags
			// We will have to properly patch that some day
			utils::hook::jump(0x140589480, get_logon_status_stub);
		}*/
	};
}

REGISTER_MODULE(arxan::module)
