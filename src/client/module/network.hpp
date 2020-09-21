#pragma once
#include "loader/module_loader.hpp"
#include "game/game.hpp"

class network final : public module
{
public:
	using callback = std::function<void(const game::native::netadr_s&, const std::string_view&)>;

	static void on(const std::string& command, const callback& callback);
	static void send(const game::native::netadr_s& address, const std::string& command, const std::string& data);
	static void send(const game::native::netadr_s& address, const std::string& data);
	
	void post_unpack() override;
};
