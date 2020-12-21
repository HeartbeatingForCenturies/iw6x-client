#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "auth.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>
#include <utils/cryptography.hpp>

#include "game/game.hpp"

namespace auth
{
	namespace
	{
		std::string get_drive_serial()
		{
			char windows_dir[1024];
			GetWindowsDirectoryA(windows_dir, sizeof(windows_dir));

			const auto* const drive = utils::string::va("%c:\\", windows_dir[0]);
		
			DWORD serial_number = 0;
			GetVolumeInformationA(drive, nullptr, NULL, &serial_number, nullptr, nullptr, nullptr, NULL);
			return std::string(reinterpret_cast<char*>(&serial_number), sizeof(serial_number));
		}

		std::string get_key_entropy()
		{
			// Weak, but enough as a first step...
			return get_drive_serial();
		}
	
		const utils::cryptography::ecc::key& get_key()
		{
			static auto key = utils::cryptography::ecc::generate_key(512, get_key_entropy());
			return key;
		}
	}

	uint64_t get_key_hash()
	{
		std::string hash = utils::cryptography::sha1::compute(get_key().get_public_key());

		if (hash.size() >= 8)
		{
			return *reinterpret_cast<uint64_t*>(const_cast<char*>(hash.data()));
		}

		return 0;
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			// Patch steam id bit check
			if(game::environment::is_sp())
			{
				utils::hook::jump(0x1404BF3F0, 0x1404BF446);
				utils::hook::jump(0x1404C02AF, 0x1404C02F0);
				utils::hook::jump(0x1404C07F4, 0x1404C0842);
			}
			else
			{
				utils::hook::jump(0x140585410, 0x140585466);
				utils::hook::jump(0x140142252, 0x140142293);
				utils::hook::jump(0x140142334, 0x140142389);
				utils::hook::jump(0x1405864EF, 0x140586530);
				utils::hook::jump(0x140586A80, 0x140586AC6);
			}
		}
	};
}

REGISTER_COMPONENT(auth::component)
