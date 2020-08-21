#pragma once

#include "structs.hpp"
#include "launcher/launcher.hpp"

#define SELECT_VALUE(sp, mp, dedi) (game::is_sp() ? (sp) : (game::is_mp() ? (mp) : (dedi)))

#define SERVER_CD_KEY "IW6x-CD-Key"

namespace game
{
	namespace native
	{
		
	}

	bool is_mp();
	bool is_sp();
	bool is_dedi();

	void initialize(launcher::mode mode);
}
