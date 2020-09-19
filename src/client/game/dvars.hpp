#pragma once

#include "game.hpp"
#include "structs.hpp"

namespace dvars
{
	extern game::native::dvar_t* con_inputBoxColor;
	extern game::native::dvar_t* con_inputHintBoxColor;
	extern game::native::dvar_t* con_outputBarColor;
	extern game::native::dvar_t* con_outputSliderColor;
	extern game::native::dvar_t* con_outputWindowColor;
	extern game::native::dvar_t* con_inputDvarMatchColor;
	extern game::native::dvar_t* con_inputDvarValueColor;
	extern game::native::dvar_t* con_inputDvarInactiveValueColor;
	extern game::native::dvar_t* con_inputCmdMatchColor;

	extern game::native::dvar_t* g_gravity;
	extern game::native::dvar_t* g_speed;
}
