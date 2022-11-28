#pragma once

namespace party
{
	void reset_connect_state();

	void connect(const game::netadr_s& target);
	void map_restart();

	int server_client_count();

	int get_client_count();
	int get_bot_count();
}
