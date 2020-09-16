#include <std_include.hpp>
#include "game_console.hpp"

#include "game/game.hpp"
#include "game/dvars.hpp"
#include "module/command.hpp"
#include "module/scheduler.hpp"

#include "utils/string.hpp"

GameConsole* game_console::con = new GameConsole;

std::int32_t game_console::history_index = -1;
std::deque<std::string> game_console::history;

std::string game_console::fixed_input;
std::vector<std::string> game_console::matches;

utils::hook::detour game_console::cl_char_event_hook;
utils::hook::detour game_console::cl_key_event_hook;
utils::hook::detour game_console::r_end_frame_hook;

float color_white[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
float color_iw6[4] = { 0.0f, 0.7f, 1.0f, 1.0f };

void game_console::clear()
{
	strncpy_s(con->buffer, "", 256);
	con->cursor = 0;

	fixed_input = "";
	matches.clear();
}

void game_console::print(std::string& data)
{
	if (con->visibleLineCount > 0 && con->displayLineOffset == (con->output.size() - con->visibleLineCount))
	{
		con->displayLineOffset++;
	}

	con->output.push_back(data);

	printf("%s\n", data.data());

	if (con->output.size() > 256)
	{
		con->output.pop_front();
	}
}

void game_console::print(int type, const char* fmt, ...)
{
	char va_buffer[0x200] = { 0 };

	va_list ap;
	va_start(ap, fmt);
	vsprintf_s(va_buffer, fmt, ap);
	va_end(ap);

	auto formatted = std::string(va_buffer);
	auto lines = utils::string::split(formatted, '\n');

	for (auto& line : lines)
	{
		print(type == CON_TYPE_INFO ? line : "^"s.append(std::to_string(type)).append(line));
	}
}

void game_console::toggle_console()
{
	clear();

	con->outputVisible = 0;
	game::native::clientUIActives->keyCatchers ^= 1;
}

void game_console::toggle_console_output()
{
	con->outputVisible = con->outputVisible == 0;
}

void game_console::check_resize()
{
	con->screenMin[0] = 6.0f;
	con->screenMin[1] = 6.0f;
	con->screenMax[0] = game::native::ScrPlace_GetViewPlacement()->realViewportSize[0] - 6.0f;
	con->screenMax[1] = game::native::ScrPlace_GetViewPlacement()->realViewportSize[1] - 6.0f;

	if (console_font)
	{
		con->fontHeight = console_font->pixelHeight;
		con->visibleLineCount = static_cast<int>((con->screenMax[1] - con->screenMin[1] - (con->fontHeight * 2)) - 24.0f) / con->fontHeight;
		con->visiblePixelWidth = static_cast<int>(((con->screenMax[0] - con->screenMin[0]) - 10.0f) - 18.0f);
	}
	else
	{
		con->fontHeight = 0;
		con->visibleLineCount = 0;
		con->visiblePixelWidth = 0;
	}
}

void game_console::draw_box(float x, float y, float w, float h, float* color)
{
	game::vec4_t dark_color;

	dark_color[0] = color[0] * 0.5;
	dark_color[1] = color[1] * 0.5;
	dark_color[2] = color[2] * 0.5;
	dark_color[3] = color[3];

	game::native::R_AddCmdDrawStretchPic(x, y, w, h, 0.0f, 0.0f, 0.0f, 0.0f, color, material_white);
	game::native::R_AddCmdDrawStretchPic(x, y, 2.0f, h, 0.0f, 0.0f, 0.0f, 0.0f, dark_color, material_white);
	game::native::R_AddCmdDrawStretchPic((x + w) - 2.0f, y, 2.0f, h, 0.0f, 0.0f, 0.0f, 0.0f, dark_color, material_white);
	game::native::R_AddCmdDrawStretchPic(x, y, w, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, dark_color, material_white);
	game::native::R_AddCmdDrawStretchPic(x, (y + h) - 2.0f, w, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, dark_color, material_white);
}

void game_console::draw_input_box([[maybe_unused]] int lines, float* color)
{
	draw_box(
		con->globals->x - 6.0f,
		con->globals->y - 6.0f,
		(con->screenMax[0] - con->screenMin[0]) - ((con->globals->x - 6.0f) - con->screenMin[0]),
		(lines * con->globals->fontHeight) + 12.0f,
		color);
}

void game_console::draw_input_text_and_over(const char* str, float* color)
{
	game::native::R_AddCmdDrawText(str, 0x7FFFFFFF, console_font, con->globals->x, con->globals->y + con->globals->fontHeight, 1.0f, 1.0f, 0.0, color, 0);
	con->globals->x = game::native::R_TextWidth(str, 0, console_font) + con->globals->x + 6.0f;
}

void game_console::draw_hint_box(int lines, float* color, float offset_x, float offset_y)
{
	float _h = lines * con->globals->fontHeight + 12.0f;
	float _y = con->globals->y - 3.0f + con->globals->fontHeight + 12.0f;
	float _w = (con->screenMax[0] - con->screenMin[0]) - ((con->globals->x - 6.0f) - con->screenMin[0]);

	draw_box(con->globals->x - 6.0f, _y, _w, _h, color);
}

void game_console::draw_hint_text(int line, const char* text, float* color, float offset)
{
	float _y = con->globals->fontHeight + con->globals->y + (con->globals->fontHeight * (line + 1)) + 15.0f;

	game::native::R_AddCmdDrawText(text, 0x7FFFFFFF, console_font, con->globals->x + offset, _y, 1.0f, 1.0f, 0.0f, color, 0);
}

bool game_console::match_compare(std::string& input, std::string& text, bool exact)
{
	if (exact && text == input) return true;
	if (!exact && text.find(input) != std::string::npos) return true;
	return false;
}

void game_console::find_matches(std::string input, std::vector<std::string>& suggestions, bool exact)
{
	std::string _input = utils::string::to_lower(input);

	for (int i = 0; i < *game::native::dvarCount; i++)
	{
		if (game::native::sortedDvars[i] && game::native::sortedDvars[i]->name)
		{
			std::string name = utils::string::to_lower(game::native::sortedDvars[i]->name);

			if (match_compare(_input, name, exact))
			{
				suggestions.push_back(game::native::sortedDvars[i]->name);
			}

			if (exact && suggestions.size() > 1)
			{
				return;

			}
		}
	}

	game::native::cmd_function_s* cmd = (*game::native::cmd_functions);
	while (cmd)
	{
		if (cmd->name)
		{
			std::string name = utils::string::to_lower(cmd->name);

			if (match_compare(_input, name, exact))
			{
				suggestions.push_back(cmd->name);
			}

			if (exact && suggestions.size() > 1)
			{
				return;
			}
		}
		cmd = cmd->next;
	}
}

void game_console::draw_input()
{
	con->globals->fontHeight = static_cast<float>(console_font->pixelHeight);
	con->globals->x = con->screenMin[0] + 6.0f;
	con->globals->y = con->screenMin[1] + 6.0f;
	con->globals->leftX = con->screenMin[0] + 6.0f;

	draw_input_box(1, dvars::con_inputBoxColor->current.vector);
	draw_input_text_and_over("IW6x >", color_iw6);

	con->globals->leftX = con->globals->x;
	con->globals->autoCompleteChoice[0] = 0;

	game::native::R_AddCmdDrawTextWithCursor(con->buffer, 0x7FFFFFFF, console_font, con->globals->x, con->globals->y + con->globals->fontHeight, 1.0f, 1.0f, 0.0f, color_white, 0, con->cursor, '|');

	// check if using a prefixed '/' or not
	std::string input = con->buffer[1] && (con->buffer[0] == '/' || con->buffer[0] == '\\') ? std::string(con->buffer).substr(1) : std::string(con->buffer);

	if (!input.length())
	{
		return;
	}

	if (input != fixed_input)
	{
		matches.clear();

		if (input.find(" ") != std::string::npos)
		{
			find_matches(input.substr(0, input.find(" ")), matches, true);
		}
		else
		{
			find_matches(input, matches, false);
		}

		fixed_input = input;
	}

	con->globals->mayAutoComplete = false;
	if (matches.size() > 24)
	{
		draw_hint_box(1, dvars::con_inputHintBoxColor->current.vector);
		draw_hint_text(0, utils::string::va("%i matches (too many to show here)", matches.size()), dvars::con_inputDvarMatchColor->current.vector);
	}
	else if (matches.size() == 1)
	{
		auto dvar = game::native::Dvar_FindVar(matches[0].data());
		int line_count = dvar ? 2 : 1;

		draw_hint_box(line_count, dvars::con_inputHintBoxColor->current.vector);
		draw_hint_text(0, matches[0].data(), dvar ? dvars::con_inputDvarMatchColor->current.vector : dvars::con_inputCmdMatchColor->current.vector);

		if (dvar)
		{
			auto offset = (con->screenMax[0] - con->globals->x) / 2.5f;

			draw_hint_text(0, game::native::Dvar_ValueToString(dvar, dvar->current), dvars::con_inputDvarValueColor->current.vector, offset);
			draw_hint_text(1, "  default", dvars::con_inputDvarInactiveValueColor->current.vector);
			draw_hint_text(1, game::native::Dvar_ValueToString(dvar, dvar->reset), dvars::con_inputDvarInactiveValueColor->current.vector, offset);
		}

		strncpy(con->globals->autoCompleteChoice, matches[0].data(), 64);
		con->globals->mayAutoComplete = true;
	}
	else if (matches.size() > 1)
	{
		draw_hint_box(static_cast<int>(matches.size()), dvars::con_inputHintBoxColor->current.vector);

		auto offset = (con->screenMax[0] - con->globals->x) / 2.5f;

		for (int i = 0; i < static_cast<int>(matches.size()); i++)
		{
			auto dvar = game::native::Dvar_FindVar(matches[i].data());

			draw_hint_text(i, matches[i].data(), dvar ? dvars::con_inputDvarMatchColor->current.vector : dvars::con_inputCmdMatchColor->current.vector);

			if (dvar)
			{
				draw_hint_text(i, game::native::Dvar_ValueToString(dvar, dvar->current), dvars::con_inputDvarValueColor->current.vector, offset);
			}
		}

		strncpy(con->globals->autoCompleteChoice, matches[0].data(), 64);
		con->globals->mayAutoComplete = true;
	}
}

void game_console::draw_output_scrollbar(float x, float y, float width, float height)
{
	float _x = (x + width) - 10.0f;
	draw_box(_x, y, 10.0f, height, dvars::con_outputBarColor->current.vector);

	float _height = height;
	if (con->output.size() > con->visibleLineCount)
	{
		float percentage = static_cast<float>(con->visibleLineCount) / con->output.size();
		_height *= percentage;

		float remainingSpace = height - _height;
		float percentageAbove = static_cast<float>(con->displayLineOffset) / (con->output.size() - con->visibleLineCount);

		y = y + (remainingSpace * percentageAbove);
	}

	draw_box(_x, y, 10.0f, _height, dvars::con_outputSliderColor->current.vector);
}

void game_console::draw_output_text(float x, float y)
{
	float offset = con->output.size() >= con->visibleLineCount ? 0.0f : (con->fontHeight * (con->visibleLineCount - con->output.size()));

	for (int i = 0; i < con->visibleLineCount; i++)
	{
		y = console_font->pixelHeight + y;

		int index = i + con->displayLineOffset;
		if (index >= con->output.size())
		{
			break;
		}

		game::native::R_AddCmdDrawText(con->output.at(index).c_str(), 0x7FFF, console_font, x, y + offset, 1.0f, 1.0f, 0.0, color_white, 0);
	}
}

void game_console::draw_output_window()
{
	draw_box(con->screenMin[0], con->screenMin[1] + 32.0f, con->screenMax[0] - con->screenMin[0], (con->screenMax[1] - con->screenMin[1]) - 32.0f, dvars::con_outputWindowColor->current.vector);

	float x = con->screenMin[0] + 6.0f;
	float y = (con->screenMin[1] + 32.0f) + 6.0f;
	float width = (con->screenMax[0] - con->screenMin[0]) - 12.0f;
	float height = ((con->screenMax[1] - con->screenMin[1]) - 32.0f) - 12.0f;

	game::native::R_AddCmdDrawText("IW6 MP 3.15 build 2 Sat Sep 14 2013 03:58:30PM win64", 0x7FFFFFFF, console_font, x, ((height - 16.0f) + y) + console_font->pixelHeight, 1.0f, 1.0f, 0.0, color_iw6, 0);

	draw_output_scrollbar(x, y, width, height);
	draw_output_text(x, y);
}

void game_console::draw_console()
{
	check_resize();

	if (game::native::clientUIActives->keyCatchers & 1)
	{
		if (!(game::native::clientUIActives->keyCatchers & 1))
		{
			con->outputVisible = 0;
		}

		if (con->outputVisible)
		{
			draw_output_window();
		}

		draw_input();
	}
}

void game_console::cl_char_event_stub(int localClientNum, int key)
{
	if (key == game::native::keyNum_t::K_GRAVE || key == game::native::keyNum_t::K_TILDE)
	{
		return;
	}

	if (game::native::clientUIActives->keyCatchers & 1)
	{
		if (key == game::native::keyNum_t::K_TAB) // tab (auto complete) 
		{
			if (con->globals->mayAutoComplete)
			{
				char firstChar = con->buffer[0];

				clear();

				if (firstChar == '\\' || firstChar == '/')
				{
					con->buffer[0] = firstChar;
					con->buffer[1] = '\0';
				}

				strncat_s(con->buffer, con->globals->autoCompleteChoice, 64);
				con->cursor = static_cast<int>(std::string(con->buffer).length());

				if (con->cursor != 254)
				{
					con->buffer[con->cursor++] = ' ';
					con->buffer[con->cursor] = '\0';
				}
			}
		}

		if (key == 'v' - 'a' + 1) // paste
		{
			char* clipboard = game::native::Sys_GetClipboardData();
			if (!clipboard)
			{
				return;
			}

			for (int i = 0; i < strlen(clipboard); i++)
			{
				cl_char_event_stub(localClientNum, clipboard[i]);
			}

			free(clipboard);
			return;
		}

		if (key == 'c' - 'a' + 1) // clear
		{
			clear();
			con->lineCount = 0;
			con->output.clear();
			history_index = -1;
			history.clear();

			return;
		}

		if (key == 'h' - 'a' + 1) // backspace
		{
			if (con->cursor > 0)
			{
				memmove(con->buffer + con->cursor - 1, con->buffer + con->cursor, strlen(con->buffer) + 1 - con->cursor);
				con->cursor--;
			}

			return;
		}

		if (key < 32)
		{
			return;
		}

		if (con->cursor == 256 - 1)
		{
			return;
		}

		memmove(con->buffer + con->cursor + 1, con->buffer + con->cursor, strlen(con->buffer) + 1 - con->cursor);
		con->buffer[con->cursor] = static_cast<char>(key);
		con->cursor++;

		if (con->cursor == strlen(con->buffer) + 1)
		{
			con->buffer[con->cursor] = 0;
		}
	}

	cl_char_event_hook.invoke<void>(localClientNum, key);
}

void game_console::cl_key_event_stub(int localClientNum, int key, int down)
{
	if (key == game::native::keyNum_t::K_GRAVE || key == game::native::keyNum_t::K_TILDE)
	{
		if (!down)
		{
			return;
		}

		if (game::native::playerKeys[localClientNum].keys[game::native::keyNum_t::K_SHIFT].down)
		{
			if (!(game::native::clientUIActives->keyCatchers & 1))
				toggle_console();

			toggle_console_output();
			return;
		}

		toggle_console();
		return;
	}

	if (game::native::clientUIActives->keyCatchers & 1)
	{
		if (down)
		{
			if (key == game::native::keyNum_t::K_UPARROW)
			{
				if (++history_index >= history.size())
				{
					history_index = static_cast<int>(history.size()) - 1;
				}

				clear();

				if (history_index != -1)
				{
					strncpy_s(con->buffer, history.at(history_index).c_str(), 0x100);
					con->cursor = static_cast<int>(strlen(con->buffer));
				}
			}
			else if (key == game::native::keyNum_t::K_DOWNARROW)
			{
				if (--history_index < -1)
				{
					history_index = -1;
				}

				clear();

				if (history_index != -1)
				{
					strncpy_s(con->buffer, history.at(history_index).c_str(), 0x100);
					con->cursor = static_cast<int>(strlen(con->buffer));
				}
			}

			if (key == game::native::keyNum_t::K_RIGHTARROW)
			{
				if (con->cursor < strlen(con->buffer))
				{
					con->cursor++;
				}

				return;
			}

			if (key == game::native::keyNum_t::K_LEFTARROW)
			{
				if (con->cursor > 0)
				{
					con->cursor--;
				}

				return;
			}

			//scroll through output
			if (key == game::native::keyNum_t::K_MWHEELUP)
			{
				if (con->output.size() > con->visibleLineCount && con->displayLineOffset > 0)
				{
					con->displayLineOffset--;
				}
			}
			else if (key == game::native::keyNum_t::K_MWHEELDOWN)
			{
				if (con->output.size() > con->visibleLineCount && con->displayLineOffset < (con->output.size() - con->visibleLineCount))
				{
					con->displayLineOffset++;
				}
			}

			if (key == game::native::keyNum_t::K_ENTER)
			{
				game::native::Cbuf_AddText(0, utils::string::va("%s \n", &con->buffer[0]));

				if (history_index != -1)
				{
					auto itr = history.begin() + history_index;

					if (*itr == con->buffer)
					{
						history.erase(history.begin() + history_index);
					}
				}

				history.push_front(con->buffer);

				print("]"s.append(con->buffer));

				if (history.size() > 10)
				{
					history.erase(history.begin() + 10);
				}

				history_index = -1;

				clear();
			}
		}
	}

	cl_key_event_hook.invoke<void>(localClientNum, key, down);
}

void game_console::r_end_frame_stub()
{
	draw_console();

	r_end_frame_hook.invoke<void>();
}

void game_console::initialize()
{
	con->initialize();

	// setup our hooks
	cl_char_event_hook.create(SELECT_VALUE(0x14023CE50, 0x1402C2AE0), cl_char_event_stub);
	cl_key_event_hook.create(SELECT_VALUE(0x14023D070, 0x1402C2CE0), cl_key_event_stub);
	r_end_frame_hook.create(SELECT_VALUE(0x140534860, 0x140601AA0), r_end_frame_stub);

	// add clear command
	command::add("clear", [&]([[maybe_unused]] command::params& params)
	{
		clear();
		con->lineCount = 0;
		con->output.clear();
		history_index = -1;
		history.clear();
	});

	// add our dvars
	dvars::con_inputBoxColor = game::native::Dvar_RegisterVec4("con_inputBoxColor", 0.2f, 0.2f, 0.2f, 0.9f, 0.0f, 1.0f, 1, "color of console input box");
	dvars::con_inputHintBoxColor = game::native::Dvar_RegisterVec4("con_inputHintBoxColor", 0.3f, 0.3f, 0.3f, 1.0f, 0.0f, 1.0f, 1, "color of console input hint box");
	dvars::con_outputBarColor = game::native::Dvar_RegisterVec4("con_outputBarColor", 0.5f, 0.5f, 0.5f, 0.6f, 0.0f, 1.0f, 1, "color of console output bar");
	dvars::con_outputSliderColor = game::native::Dvar_RegisterVec4("con_outputSliderColor", 0.0f, 0.7f, 1.0f, 1.00f, 0.0f, 1.0f, 1, "color of console output slider");
	dvars::con_outputWindowColor = game::native::Dvar_RegisterVec4("con_outputWindowColor", 0.25f, 0.25f, 0.25f, 0.85f, 0.0f, 1.0f, 1, "color of console output window");
	dvars::con_inputDvarMatchColor = game::native::Dvar_RegisterVec4("con_inputDvarMatchColor", 1.0f, 1.0f, 0.8f, 1.0f, 0.0f, 1.0f, 1, "color of console matched dvar");
	dvars::con_inputDvarValueColor = game::native::Dvar_RegisterVec4("con_inputDvarValueColor", 1.0f, 1.0f, 0.8f, 1.0f, 0.0f, 1.0f, 1, "color of console matched dvar value");
	dvars::con_inputDvarInactiveValueColor = game::native::Dvar_RegisterVec4("con_inputDvarInactiveValueColor", 0.8f, 0.8f, 0.8f, 1.0f, 0.0f, 1.0f, 1, "color of console inactive dvar value");
	dvars::con_inputCmdMatchColor = game::native::Dvar_RegisterVec4("con_inputCmdMatchColor", 0.80f, 0.80f, 1.0f, 1.0f, 0.0f, 1.0f, 1, "color of console matched command");
}

ATOM register_class_ex_a(const WNDCLASSEXA* Arg1)
{
	// doing this from in here since we cant hook until game is fully unpacked
	game_console::initialize();

	return RegisterClassExA(Arg1);
}

game_console::game_console()
{

}

void* game_console::load_import(const std::string& module, const std::string& function)
{
	if (module == "USER32.dll" && function == "RegisterClassExA")
	{
		return register_class_ex_a;
	}

	return nullptr;
}

REGISTER_MODULE(game_console);