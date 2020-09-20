#pragma once
#include "loader/module_loader.hpp"
#include "utils/hook.hpp"

#include "game/game.hpp"

class server_list final : public module
{	
public:
	void post_unpack() override;

private:
	static void lui_open_menu_stub(int controllerIndex, const char* menu, int a3, int a4, unsigned int a5);

};
