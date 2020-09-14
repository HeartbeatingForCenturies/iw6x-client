#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdlib>

namespace
{
	void run_watchdog()
	{
		auto* const command = "-proc ";
		const char* parent_proc = strstr(GetCommandLineA(), command);

		if (parent_proc)
		{
			const auto pid = atoi(parent_proc + strlen(command));
			auto* const process_handle = OpenProcess(SYNCHRONIZE, FALSE, pid);
			if (process_handle && process_handle != INVALID_HANDLE_VALUE)
			{
				WaitForSingleObject(process_handle, INFINITE);
				CloseHandle(process_handle);
			}

			TerminateProcess(GetCurrentProcess(), 0);
		}
	}
}

int __stdcall WinMain(HINSTANCE, HINSTANCE, PSTR, int)
{
	run_watchdog();
	return 0;
}
