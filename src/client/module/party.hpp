#pragma once
#include "loader/module_loader.hpp"
#include "game/game.hpp"

class party final : public module
{
public:
	void post_unpack() override;

	static void connect(const game::netadr_s& target);
};
