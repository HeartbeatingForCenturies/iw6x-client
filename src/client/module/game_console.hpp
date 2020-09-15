#pragma once
#include "loader/module_loader.hpp"
#include "utils/hook.hpp"

#define console_font game::native::R_RegisterFont("fonts/consolefont")
#define material_white game::native::Material_RegisterHandle("white")

#define color_white new float[4] { 1.0f, 1.0f, 1.0f, 1.0f }
#define color_iw6 new float[4] { 0.0f, 0.7f, 1.0f, 1.0f }

#define DARK_COLOR(color) new float[4] { 0.5f * color[0], 0.5f * color[1], 0.5f * color[2], 1.0f }

enum ConsoleType
{
	CON_TYPE_ERROR = 1,
	CON_TYPE_WARN = 3,
	CON_TYPE_INFO = 7
};

struct ConsoleGlobals
{
	float x;
	float y;
	float leftX;
	float fontHeight;
	bool mayAutoComplete;
	char autoCompleteChoice[64];
	int infoLineCount;

	inline void initialize()
	{
		this->x = 0.0f;
		this->y = 0.0f;
		this->leftX = 0.0f;
		this->fontHeight = 0.0f;
		this->mayAutoComplete = false;
		strncpy(this->autoCompleteChoice, "", 64);
		this->infoLineCount = 0;
	}
};

struct GameConsole
{
	char buffer[256];
	int cursor;
	int fontHeight;
	int visibleLineCount;
	int visiblePixelWidth;
	float screenMin[2]; //left & top
	float screenMax[2]; //right & bottom
	ConsoleGlobals* globals;
	bool outputVisible;
	int displayLineOffset;
	int lineCount;
	std::deque<std::string> output;

	inline void initialize()
	{
		strncpy(this->buffer, "", 256);
		this->cursor = 0;
		this->visibleLineCount = 0;
		this->globals = new ConsoleGlobals();
		this->globals->initialize();
		this->outputVisible = false;
		this->displayLineOffset = 0;
		this->lineCount = 0;
	}
};

class game_console final : public module
{	
public:
	game_console();

	void* load_import(const std::string& module, const std::string& function) override;

	static void initialize();
	static void print(int type, const char* fmt, ...);
	static void draw_console();

	static std::string fixed_input;
	static std::vector<std::string> matches;

	static GameConsole* con;

private:

	static std::int32_t history_index;
	static std::deque<std::string> history;

	static utils::hook::detour cl_char_event_hook;
	static void cl_char_event_stub(int localClientNum, int key);

	static utils::hook::detour cl_key_event_hook;
	static void cl_key_event_stub(int localClientNum, int key, int down);

	static utils::hook::detour r_end_frame_hook;
	static void r_end_frame_stub();

	static void print(std::string& data);

	static void clear();
	static void toggle_console();
	static void toggle_console_output();

	static void check_resize();

	static void draw_box(float x, float y, float w, float h, float* color);

	static void draw_output_text(float x, float y);
	static void draw_output_scrollbar(float x, float y, float width, float height);
	static void draw_output_window();

	static bool match_compare(std::string& input, std::string& text, bool exact);
	static void find_matches(const std::string input, std::vector<std::string>& suggestions, bool exact);
	
	//static std::string dvar_get_vector_domain(int components, const game::DvarLimits& domain);
	//static std::string dvar_get_domain(game::dvar_type type, const game::DvarLimits& domain, int* outLineCount);

	static void draw_hint_box(int lines, float* color, float offset_x = 0.0f, float offset_y = 0.0f);
	static void draw_hint_text(int line, const char* text, float* color, float offset = 0.0f);
	static void draw_input_box(int lines, float* color);
	static void draw_input_text_and_over(const char* str, float* color);
	static void draw_input();
};
