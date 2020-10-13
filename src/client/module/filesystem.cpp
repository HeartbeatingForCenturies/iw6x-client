#include <std_include.hpp>
#include "filesystem.hpp"

#include "command.hpp"
#include "game/game.hpp"
#include "utils/hook.hpp"

filesystem::file::file(std::string name)
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

bool filesystem::file::exists() const
{
	return this->valid_;
}

const std::string& filesystem::file::get_buffer() const
{
	return this->buffer_;
}

const std::string& filesystem::file::get_name() const
{
	return this->name_;
}

void filesystem::post_unpack()
{
	// Set fs_basegame
	utils::hook::inject(SELECT_VALUE(0x14041C053, 0x1404DDA13), "iw6x");
}

REGISTER_MODULE(filesystem)
