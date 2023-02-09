#pragma once

namespace party
{
	void reset_connect_state();

	void connect(const game::netadr_s& target);
	void map_restart();

	[[nodiscard]] int server_client_count();

	[[nodiscard]] int get_client_count();
	[[nodiscard]] int get_bot_count();

	[[nodiscard]] game::netadr_s& get_target();
}
