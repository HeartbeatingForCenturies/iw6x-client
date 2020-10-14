#include <std_include.hpp>
#include "loader/module_loader.hpp"

#include "game/game.hpp"
#include "game/dvars.hpp"

#include "utils/hook.hpp"

namespace colors
{
	namespace
	{
		std::vector<DWORD> color_table;

		DWORD hsv_to_rgb(const game::HsvColor hsv)
		{
			DWORD rgb;

			if (hsv.s == 0)
			{
				return RGB(hsv.v, hsv.v, hsv.v);
			}

			// converting to 16 bit to prevent overflow
			const unsigned int h = hsv.h;
			const unsigned int s = hsv.s;
			const unsigned int v = hsv.v;

			const auto region = static_cast<uint8_t>(h / 43);
			const auto remainder = (h - (region * 43)) * 6;

			const auto p = static_cast<uint8_t>((v * (255 - s)) >> 8);
			const auto q = static_cast<uint8_t>(
				(v * (255 - ((s * remainder) >> 8))) >> 8);
			const auto t = static_cast<uint8_t>(
				(v * (255 - ((s * (255 - remainder)) >> 8))) >> 8);

			switch (region)
			{
			case 0:
				rgb = RGB(v, t, p);
				break;
			case 1:
				rgb = RGB(q, v, p);
				break;
			case 2:
				rgb = RGB(p, v, t);
				break;
			case 3:
				rgb = RGB(p, q, v);
				break;
			case 4:
				rgb = RGB(t, p, v);
				break;
			default:
				rgb = RGB(v, p, q);
				break;
			}

			return rgb;
		}

		char color_index(const char c)
		{
			const char index = c - 48;
			return index >= 0xC ? 7 : index;
		}

		void strip(const char* in, char* out, int max)
		{
			if (!in || !out) return;

			max--;
			auto current = 0;
			while (*in != 0 && current < max)
			{
				const auto index = *(in + 1);
				if (*in == '^' && (color_index(index) != 7 || index == '7'))
				{
					++in;
				}
				else
				{
					*out = *in;
					++out;
					++current;
				}

				++in;
			}
			*out = '\0';
		}

		char add(const uint8_t r, const uint8_t g, const uint8_t b)
		{
			const char index = '0' + static_cast<char>(color_table.size());
			color_table.push_back(RGB(r, g, b));
			return index;
		}

		void com_clean_name_stub(const char* in, char* out, const int out_size)
		{
			strncpy_s(out, out_size, in, _TRUNCATE);
		}

		char* i_clean_str_stub(char* string)
		{
			strip(string, string, static_cast<int>(strlen(string)) + 1);

			return string;
		}

		void rb_lookup_color_stub(const char index, DWORD* color)
		{
			*color = RGB(255, 255, 255);

			if (index == '8')
			{
				*color = *reinterpret_cast<DWORD*>(SELECT_VALUE(0x145FFD958, 0x1480E85BC));
			}
			else if (index == '9')
			{
				*color = *reinterpret_cast<DWORD*>(SELECT_VALUE(0x145FFD95C, 0x1480E85C0));
			}
			else if (index == ':')
			{
				*color = hsv_to_rgb({static_cast<uint8_t>((game::Sys_Milliseconds() / 100) % 256), 255, 255});
			}
			else if (index == ';')
			{
				*color = *reinterpret_cast<DWORD*>(SELECT_VALUE(0x145FFD964, 0x1480E85C8));
			}
			else
			{
				*color = color_table[color_index(index)];
			}
		}
	}

	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_dedi())
			{
				return;
			}

			if (!game::environment::is_sp())
			{
				// allows colored name in-game
				utils::hook::jump(0x1404F5FC0, com_clean_name_stub);

				// patch I_CleanStr
				utils::hook::jump(0x1404F63C0, i_clean_str_stub);
			}

			// force new colors
			utils::hook::jump(SELECT_VALUE(0x14055DCC0, 0x14062AE80), rb_lookup_color_stub);

			// add colors
			add(0, 0, 0); // 0  - Black
			add(255, 49, 49); // 1  - Red
			add(134, 192, 0); // 2  - Green
			add(255, 173, 34); // 3  - Yellow
			add(0, 135, 193); // 4  - Blue
			add(32, 197, 255); // 5  - Light Blue
			add(151, 80, 221); // 6  - Pink
			add(255, 255, 255); // 7  - White

			add(0, 0, 0); // 8  - Team color (axis?)
			add(0, 0, 0); // 9  - Team color (allies?)

			add(0, 0, 0); // 10 - Rainbow (:)
			add(0, 0, 0);
			// 11 - Server color (;) - using that color in infostrings (e.g. your name) fails, ';' is an illegal character!
		}
	};
}

REGISTER_MODULE(colors::module)
