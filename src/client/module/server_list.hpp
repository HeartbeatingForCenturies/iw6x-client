#pragma once
#include "loader/module_loader.hpp"
#include "utils/hook.hpp"

#include "game/game.hpp"

#define MAX_SERVERS 20

struct server_info
{
    // gotta add more to this
    char clients;
    char max_clients;
    short ping;
    char host_name[32];
    char map_name[32];
    char game_type[16];
    char in_game;
};

class server_list final : public module
{	
public:
	void post_unpack() override;

    static bool update_server_list;
    static int server_count;

private:
    static int ui_feeder_count();
    static const char* ui_feeder_item_text(int localClientNum, void* a2, void* a3, int index, int column);

    static int display_servers[MAX_SERVERS];
    static server_info servers[MAX_SERVERS];

    static void add_server();
    static void insert_server(server_info* server);

};
