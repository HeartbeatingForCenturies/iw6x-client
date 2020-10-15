#pragma once
#include "game/game.hpp"

namespace network
{
	using callback = std::function<void(const game::netadr_s&, const std::string_view&)>;

	void on(const std::string& command, const callback& callback);
	void send(const game::netadr_s& address, const std::string& command, const std::string& data);
	void send(const game::netadr_s& address, const std::string& data);

	bool are_addresses_equal(const game::netadr_s& a, const game::netadr_s& b);
}

inline bool operator==(const game::netadr_s& a, const game::netadr_s& b)
{
	return network::are_addresses_equal(a, b); //
}

inline bool operator!=(const game::netadr_s& a, const game::netadr_s& b)
{
	return !(a == b); //
}
