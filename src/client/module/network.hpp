#pragma once
#include "loader/module_loader.hpp"
#include "game/game.hpp"

class network final : public module_interface
{
public:
	using callback = std::function<void(const game::netadr_s&, const std::string_view&)>;

	static void on(const std::string& command, const callback& callback);
	static void send(const game::netadr_s& address, const std::string& command, const std::string& data);
	static void send(const game::netadr_s& address, const std::string& data);

	static bool are_addresses_equal(const game::netadr_s& a, const game::netadr_s& b);

	void post_unpack() override;
};
