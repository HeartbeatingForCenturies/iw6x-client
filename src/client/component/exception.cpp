#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "system_check.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/io.hpp>
#include <utils/string.hpp>

#include <exception/minidump.hpp>

#include <version.hpp>

namespace exception
{
	namespace
	{
		void display_error_dialog(const LPEXCEPTION_POINTERS exceptioninfo)
		{
			std::string error_str = "Termination because of a stack overflow.";
			if (exceptioninfo->ExceptionRecord->ExceptionCode != EXCEPTION_STACK_OVERFLOW)
			{
				error_str = utils::string::va("Fatal error (0x%08X) at 0x%p.",
				                              exceptioninfo->ExceptionRecord->ExceptionCode,
				                              exceptioninfo->ExceptionRecord->ExceptionAddress);

				error_str += "\n\n";
				if (!system_check::is_valid())
				{
					error_str += "Make sure to get supported game files to avoid such crashes!";
				}
				else
				{
					error_str += "Make sure to update your graphics card drivers and install operating system updates!";
				}
			}

			MessageBoxA(nullptr, error_str.data(), "ERROR", MB_ICONERROR);
		}

		std::string get_exe_filename()
		{
			char exe_file_name[MAX_PATH] = {0};

			GetModuleFileNameA(nullptr, exe_file_name, MAX_PATH);
			PathStripPathA(exe_file_name);
			PathRemoveExtensionA(exe_file_name);

			return exe_file_name;
		}

		std::string get_timestamp()
		{
			tm ltime{};
			char timestamp[MAX_PATH] = {0};
			const auto time = _time64(nullptr);

			_localtime64_s(&ltime, &time);
			strftime(timestamp, sizeof(timestamp) - 1, "%Y%m%d%H%M%S", &ltime);

			return timestamp;
		}

		std::string generate_minidump_filename()
		{
			const auto exe_ename = get_exe_filename();
			const auto timestamp = get_timestamp();

			char filepath[MAX_PATH] = {0};
			utils::io::create_directory("minidumps");
			const auto* const filename = utils::string::va("%s-%d-%s-%s.dmp", exe_ename.data(),
			                                               game::environment::get_mode(),
			                                               VERSION,
			                                               timestamp.data());

			PathCombineA(filepath, "minidumps\\", filename);

			return filepath;
		}

		bool is_harmless_error(const LPEXCEPTION_POINTERS exceptioninfo)
		{
			const auto code = exceptioninfo->ExceptionRecord->ExceptionCode;
			return code == STATUS_INTEGER_OVERFLOW || code == STATUS_FLOAT_OVERFLOW;
		}


		void write_minidump(const LPEXCEPTION_POINTERS exceptioninfo, const std::string& filename)
		{
			const auto data = create_minidump(exceptioninfo);
			//data = utils::compression::zlib::compress(data);
			utils::io::write_file(filename, data);
		}

		void suspend_other_threads()
		{
			auto* const h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
			if (h != INVALID_HANDLE_VALUE)
			{
				const auto _ = gsl::finally([h]()
				{
					CloseHandle(h);
				});

				THREADENTRY32 entry;
				entry.dwSize = sizeof(entry);
				if (!Thread32First(h, &entry))
				{
					return;
				}

				do
				{
					const auto check_size = entry.dwSize < FIELD_OFFSET(THREADENTRY32, th32OwnerProcessID) + sizeof(
						entry.th32OwnerProcessID);
					entry.dwSize = sizeof(entry);

					if (!check_size)
					{
						continue;
					}

					if (entry.th32ThreadID != GetCurrentThreadId() || entry.th32OwnerProcessID ==
						GetCurrentProcessId())
					{
						const auto thread = OpenThread(THREAD_ALL_ACCESS, FALSE, entry.th32ThreadID);
						if (thread != nullptr)
						{
							SuspendThread(thread);
							CloseHandle(thread);
						}
					}
				}
				while (Thread32Next(h, &entry));
			}
		}

		void show_mouse_cursor()
		{
			while (ShowCursor(TRUE) < 0);
		}

		LONG WINAPI exception_filter(const LPEXCEPTION_POINTERS exceptioninfo)
		{
			{
				if (is_harmless_error(exceptioninfo))
				{
					return EXCEPTION_CONTINUE_EXECUTION;
				}

				suspend_other_threads();
				show_mouse_cursor();

				display_error_dialog(exceptioninfo);
				const auto filename = generate_minidump_filename();
				write_minidump(exceptioninfo, filename);
			}

			TerminateProcess(GetCurrentProcess(), exceptioninfo->ExceptionRecord->ExceptionCode);
			return EXCEPTION_CONTINUE_SEARCH;
		}

		LPTOP_LEVEL_EXCEPTION_FILTER WINAPI set_unhandled_exception_filter_stub(LPTOP_LEVEL_EXCEPTION_FILTER)
		{
			// Don't register anything here...
			return &exception_filter;
		}
	}

	class component final : public component_interface
	{
	public:
		void post_load() override
		{
			SetUnhandledExceptionFilter(exception_filter);
			utils::hook::jump(SetUnhandledExceptionFilter, set_unhandled_exception_filter_stub, true);
		}
	};
}

REGISTER_COMPONENT(exception::component)
