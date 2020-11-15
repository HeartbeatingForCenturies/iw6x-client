#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "filesystem.hpp"

#include "game/game.hpp"
#include "utils/hook.hpp"

namespace filesystem
{
	file::file(std::string name)
		: name_(std::move(name))
	{
		char* buffer{};
		const auto size = game::FS_ReadFile(this->name_.data(), &buffer);

		if (size >= 0)
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
		}
	};
}

REGISTER_COMPONENT(filesystem::component)
