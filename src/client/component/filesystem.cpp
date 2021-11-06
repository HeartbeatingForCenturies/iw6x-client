#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "filesystem.hpp"
#include "game_module.hpp"

#include "game/game.hpp"

#include <utils/hook.hpp>
#include <utils/string.hpp>

namespace filesystem
{
	namespace
	{
		bool custom_path_registered = false;

		std::string get_binary_directory()
		{
			const auto dir = game_module::get_host_module().get_folder();
			return utils::string::replace(dir, "/", "\\");
		}

		void register_custom_path_stub(const char* path, const char* dir)
		{
			if (!custom_path_registered)
			{
				custom_path_registered = true;

				const auto launcher_dir = get_binary_directory();
				game::FS_AddLocalizedGameDirectory(launcher_dir.data(), "data");
			}

			game::FS_AddLocalizedGameDirectory(path, dir);
		}

		void fs_startup_stub(const char* gamename)
		{
			custom_path_registered = false;
			game::FS_Startup(gamename);
		}
	}

	file::file(std::string name)
		: name_(std::move(name))
	{
		char* buffer{};
		const auto size = game::FS_ReadFile(this->name_.data(), &buffer);

		if (size >= 0 && buffer)
		{
			this->valid_ = true;
			this->buffer_.append(buffer, size);
			game::FS_FreeFile(buffer);
		}
	}

	bool file::exists() const
	{
		return this->valid_;
	}

	const std::string& file::get_buffer() const
	{
		return this->buffer_;
	}

	const std::string& file::get_name() const
	{
		return this->name_;
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			// Set fs_basegame
			utils::hook::inject(SELECT_VALUE(0x14041C053, 0x1404DDA13), "iw6x");

			if (game::environment::is_sp())
			{
				utils::hook::call(0x14041B744, fs_startup_stub);

				utils::hook::call(0x14041CD00, register_custom_path_stub);
				utils::hook::call(0x14041CD20, register_custom_path_stub);
				utils::hook::call(0x14041CD5F, register_custom_path_stub);
			}
			else
			{
				utils::hook::call(0x1404DD704, fs_startup_stub);
				utils::hook::call(0x1404DDB43, fs_startup_stub);

				utils::hook::call(0x1404DE550, register_custom_path_stub);
				utils::hook::call(0x1404DE570, register_custom_path_stub);
				utils::hook::call(0x1404DE5AF, register_custom_path_stub);
			}
		}
	};
}

REGISTER_COMPONENT(filesystem::component)
