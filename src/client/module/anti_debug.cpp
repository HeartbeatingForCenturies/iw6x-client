#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "utils/hook.hpp"

namespace
{
	typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFORMATION
	{
		BOOLEAN Inherit;
		BOOLEAN ProtectFromClose;
	} OBJECT_HANDLE_ATTRIBUTE_INFORMATION, *POBJECT_HANDLE_ATTRIBUTE_INFORMATION;

	utils::hook::detour nt_close_hook;
	utils::hook::detour nt_query_information_process;

	NTSTATUS WINAPI nt_query_information_process_stub(const HANDLE handle, const PROCESSINFOCLASS info_class, PVOID info,
	                                                  const ULONG info_length, const PULONG ret_length)
	{
		// ProcessDebugObjectHandle
		if (info_class == 30) return 0xC0000353;
		
		auto* orig = static_cast<decltype(NtQueryInformationProcess)*>(nt_query_information_process.get_original());
		return orig(handle, info_class, info, info_length, ret_length);
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
}

class anti_debug final : public module
{
public:
	void post_load() override
	{
		const PPEB peb = PPEB(__readgsqword(0x60));
		peb->BeingDebugged = false;
		*PDWORD(LPSTR(peb) + 0xBC) &= ~0x70;

		const utils::nt::module ntdll("ntdll.dll");
		nt_close_hook.create(ntdll.get_proc<void*>("NtClose"), nt_close_stub);
		nt_query_information_process.create(ntdll.get_proc<void*>("NtQueryInformationProcess"),
		                                                   nt_query_information_process_stub);
	}
};

REGISTER_MODULE(anti_debug)
