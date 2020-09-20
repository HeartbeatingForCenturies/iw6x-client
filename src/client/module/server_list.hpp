#pragma once
#include "loader/module_loader.hpp"
#include "utils/hook.hpp"

#include "game/game.hpp"

class server_list final : public module
{	
public:
	void post_unpack() override;

private:

};
