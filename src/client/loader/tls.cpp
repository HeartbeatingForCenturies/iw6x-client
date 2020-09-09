#include <std_include.hpp>
#include "tls.hpp"

#include "utils/nt.hpp"
#include "utils/io.hpp"

namespace tls
{
	namespace
	{
		std::string load_tls_dll_resource()
		{
			auto* const res = FindResource(::utils::nt::module(), MAKEINTRESOURCE(TLS_DLL), RT_RCDATA);
			if (!res) return {};

			auto* const handle = LoadResource(nullptr, res);
			if (!handle) return {};

			return std::string(LPSTR(LockResource(handle)), SizeofResource(nullptr, res));
		}

		const std::string& get_tls_dll()
		{
			static auto tls_dll = load_tls_dll_resource();
			if (tls_dll.empty())
			{
				throw std::runtime_error("Unable to load TLS DLL resource");
			}

			return tls_dll;
		}

		std::string get_temp_folder()
		{
			char path[MAX_PATH] = {0};
			if (!GetTempPathA(sizeof(path), path))
			{
				throw std::runtime_error("Unable to get temp path");
			}

			return path;
		}

		std::string write_tls_dll()
		{
			const auto& tls_dll = get_tls_dll();
			const auto temp = get_temp_folder();
			const auto dll_path = temp + "tlsdll.dll";

			std::string dll_data;
			if (!utils::io::read_file(dll_path, &dll_data))
			{
				if (!utils::io::write_file(dll_path, tls_dll))
				{
					throw std::runtime_error("Failed to write TLS DLL");
				}

				return dll_path;
			}

			if (dll_data == tls_dll || utils::io::write_file(dll_path, tls_dll))
			{
				return dll_path;
			}

			throw std::runtime_error("TLS DLL already written, but still in use. Please close IW6x and restart it.");
		}
	}


	PIMAGE_TLS_DIRECTORY allocate_tls_index()
	{
		static auto already_allocated = false;
		if (already_allocated)
		{
			throw std::runtime_error("Currently only a single allocation is supported!");
		}

		already_allocated = true;

		const auto dll_path = write_tls_dll();
		const auto tls_dll = utils::nt::module::load(dll_path);
		if (!tls_dll)
		{
			throw std::runtime_error("Failed to load TLS DLL");
		}

		return reinterpret_cast<PIMAGE_TLS_DIRECTORY>(tls_dll.get_ptr() + tls_dll.get_optional_header()
			->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
	}
}
