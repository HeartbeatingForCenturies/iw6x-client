#include <std_include.hpp>
#include "game.hpp"

namespace dvars
{
	game::native::dvar_t* con_inputBoxColor = nullptr;
	game::native::dvar_t* con_inputHintBoxColor = nullptr;
	game::native::dvar_t* con_outputBarColor = nullptr;
	game::native::dvar_t* con_outputSliderColor = nullptr;
	game::native::dvar_t* con_outputWindowColor = nullptr;
	game::native::dvar_t* con_inputDvarMatchColor = nullptr;
	game::native::dvar_t* con_inputDvarValueColor = nullptr;
	game::native::dvar_t* con_inputDvarInactiveValueColor = nullptr;
	game::native::dvar_t* con_inputCmdMatchColor = nullptr;

	game::native::dvar_t* g_gravity = nullptr;
	game::native::dvar_t* g_speed = nullptr;
}
