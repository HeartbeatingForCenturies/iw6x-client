#pragma once
#include "loader/module_loader.hpp"
#include "utils/hook.hpp"

#define console_font game::R_RegisterFont("fonts/consolefont")
#define material_white game::Material_RegisterHandle("white")

enum console_type
{
	con_type_error = 1,
	con_type_warning = 3,
	con_type_info = 7
};

struct console_globals
{
	float x;
	float y;
	float left_x;
	float font_height;
	bool may_auto_complete;
	char auto_complete_choice[64];
	int info_line_count;
};

struct ingame_console
{
	char buffer[256];
	int cursor;
	int font_height;
	int visible_line_count;
	int visible_pixel_width;
	float screen_min[2]; //left & top
	float screen_max[2]; //right & bottom
	console_globals globals;
	bool output_visible;
	int display_line_offset;
	int line_count;
	std::deque<std::string> output;
};

class game_console final : public module_interface
{
public:
	void post_load() override;
	void post_unpack() override;

	static void print(int type, const char* fmt, ...);
	static void draw_console();

	static std::string fixed_input;
	static std::vector<std::string> matches;

	static ingame_console con;

private:

	static std::int32_t history_index;
	static std::deque<std::string> history;

	static utils::hook::detour cl_char_event_hook;
	static void cl_char_event_stub(int localClientNum, int key);

	static utils::hook::detour cl_key_event_hook;
	static void cl_key_event_stub(int localClientNum, int key, int down);

	static void print(const std::string& data);

	static void clear();
	static void toggle_console();
	static void toggle_console_output();

	static void check_resize();

	static void draw_box(float x, float y, float w, float h, float* color);

	static void draw_output_text(float x, float y);
	static void draw_output_scrollbar(float x, float y, float width, float height);
	static void draw_output_window();

	static bool match_compare(const std::string& input, const std::string& text, bool exact);
	static void find_matches(std::string input, std::vector<std::string>& suggestions, bool exact);

	static void draw_hint_box(int lines, float* color, float offset_x = 0.0f, float offset_y = 0.0f);
	static void draw_hint_text(int line, const char* text, float* color, float offset = 0.0f);
	static void draw_input_box(int lines, float* color);
	static void draw_input_text_and_over(const char* str, float* color);
	static void draw_input();
};
