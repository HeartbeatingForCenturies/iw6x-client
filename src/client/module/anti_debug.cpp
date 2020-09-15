#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "utils/hook.hpp"
#include "utils/string.hpp"
#include "utils/io.hpp"

namespace
{
	typedef struct _OBJECT_HANDLE_ATTRIBUTE_INFORMATION
	{
		BOOLEAN Inherit;
		BOOLEAN ProtectFromClose;
	} OBJECT_HANDLE_ATTRIBUTE_INFORMATION, *POBJECT_HANDLE_ATTRIBUTE_INFORMATION;

	utils::hook::detour nt_close_hook;
	utils::hook::detour nt_query_information_process;
	utils::hook::detour virtual_protect_hook;

	const uint64_t text_start = 0x140001000;
	const uint64_t text_end = 0x140817000;
	const auto text_size = text_end - text_start;
	void* const text_start_ptr = reinterpret_cast<void*>(text_start);

	bool is_in_text(const uint64_t addr)
	{
		return addr >= text_start && addr < text_end;
	}

	bool is_in_text(void* addr)
	{
		return is_in_text(uint64_t(addr));
	}


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

	BOOL original_virtual_protect(const LPVOID address, const SIZE_T size, const DWORD new_protect,
	                              const PDWORD old_protect)
	{
		auto* orig = static_cast<decltype(VirtualProtect)*>(virtual_protect_hook.get_original());
		return orig(address, size, new_protect, old_protect);
	}

	BOOL WINAPI virtual_protect_stub(const LPVOID address, const SIZE_T size, DWORD new_protect,
	                                 const PDWORD old_protect)
	{
		if (is_in_text(address))
		{
			new_protect &= PAGE_EXECUTE_READ;
		}

		return original_virtual_protect(address, size, new_protect, old_protect);
	}

	void log_text_segment_change(bool write_diff_map = false)
	{
		static std::string last_segment;
		if (last_segment.empty())
		{
			last_segment.append(static_cast<char*>(text_start_ptr), text_size);
		}

		static unsigned char* diff_map = nullptr;
		if (!diff_map)
		{
			diff_map = new unsigned char[text_size];
			memset(diff_map, 0, text_size);
		}

		for (auto i = 0ull; i < text_size; i += 16)
		{
			const auto _old = _mm_loadu_si128(reinterpret_cast<const __m128i*>(last_segment.data() + i));
			const auto _new = _mm_loadu_si128(reinterpret_cast<const __m128i*>(static_cast<char*>(text_start_ptr) + i));

			const auto comparison = _mm_xor_si128(_old, _new);
			if (!_mm_test_all_zeros(comparison, comparison))
			{
				// This could be done in a more efficient way, but that's ok for now...
				for (auto j = 0ull; j < 16ull; ++j)
				{
					const auto index = i + j;
					const auto current = static_cast<char*>(text_start_ptr)[index];
					if (current != last_segment[index])
					{
						last_segment[index] = current;
						if (index < (0x140225E38 - text_start) || index > (0x140225E3F - text_start))
						{
							write_diff_map |= (diff_map[index]++ != 0);
							if (diff_map[index] > 1)
							{
								OutputDebugStringA(
									utils::string::va("%llX\n", &static_cast<char*>(text_start_ptr)[index]));
							}
						}
					}
				}
			}
		}

		if (write_diff_map)
		{
			static int i = 0;
			utils::io::write_file(utils::string::va("arxan/diff_map_%i.bin", i++),
			                      last_segment);
			memset(diff_map, 0, text_size);
		}
	}

	LONG analysis_filter(LPEXCEPTION_POINTERS info)
	{
		static auto continue_ = true;
		static auto did_use_single_step = false;
		static auto do_log = true;
		static auto last_address = text_start;
		DWORD old_protection;

		if (info->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
		{
			if (continue_)
			{
				return EXCEPTION_CONTINUE_SEARCH;
			}

			continue_ = true;

			if (do_log)
			{
				log_text_segment_change();
			}

			do_log = true;

			// Disable single step execution
			if (!did_use_single_step)
			{
				info->ContextRecord->EFlags &= ~0x100;
			}

			original_virtual_protect(text_start_ptr, text_size, PAGE_EXECUTE_READ, &old_protection);

			if (did_use_single_step)
			{
				did_use_single_step = false;
				return EXCEPTION_CONTINUE_SEARCH;
			}

			return EXCEPTION_CONTINUE_EXECUTION;
		}

		if (info->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION)
		{
			continue_ = false;
			const auto address = info->ExceptionRecord->ExceptionInformation[1];

			if (address == last_address + 1)
			{
				do_log = false;
			}

			last_address = address;

			if (!is_in_text(address))
			{
				return EXCEPTION_CONTINUE_SEARCH;
			}

			// Hm...
			if (address >= 0x140225E38 && address <= 0x140225E3F)
			{
				do_log = false;
			}

			did_use_single_step = false;
			if (info->ContextRecord->EFlags & 0x100)
			{
				did_use_single_step = true;
			}

			// Enable single step execution
			info->ContextRecord->EFlags |= 0x100;

			original_virtual_protect(text_start_ptr, text_size, PAGE_EXECUTE_READWRITE, &old_protection);

			return EXCEPTION_CONTINUE_EXECUTION;
		}

		return (info->ExceptionRecord->ExceptionCode == STATUS_INVALID_HANDLE)
			       ? EXCEPTION_CONTINUE_EXECUTION
			       : EXCEPTION_CONTINUE_SEARCH;
	}

	LONG WINAPI exception_filter(LPEXCEPTION_POINTERS info)
	{
		return (info->ExceptionRecord->ExceptionCode == STATUS_INVALID_HANDLE)
			       ? EXCEPTION_CONTINUE_EXECUTION
			       : EXCEPTION_CONTINUE_SEARCH;
	}
}

class anti_debug final : public module
{
public:
	~anti_debug()
	{
#if defined(DEV_BUILD) && 0
		log_text_segment_change(true);
#endif
	}

	void post_load() override
	{
		auto* const peb = PPEB(__readgsqword(0x60));
		peb->BeingDebugged = false;
		*PDWORD(LPSTR(peb) + 0xBC) &= ~0x70;

		const utils::nt::module ntdll("ntdll.dll");
		nt_close_hook.create(ntdll.get_proc<void*>("NtClose"), nt_close_stub);
		nt_query_information_process.create(ntdll.get_proc<void*>("NtQueryInformationProcess"),
		                                    nt_query_information_process_stub);

#if defined(DEV_BUILD) && 0
		virtual_protect_hook.create(VirtualProtect, virtual_protect_stub);
		AddVectoredExceptionHandler(1, analysis_filter);

		log_text_segment_change();

		DWORD old_protection;
		original_virtual_protect(text_start_ptr, text_size, PAGE_EXECUTE_READ, &old_protection);
#else
		AddVectoredExceptionHandler(1, exception_filter);
#endif
	}
};

REGISTER_MODULE(anti_debug)
