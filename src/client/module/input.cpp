#include <std_include.hpp>
#include "loader/module_loader.hpp"

#include "game/game.hpp"

#include "game_console.hpp"
#include "server_list.hpp"

#include "utils/hook.hpp"

namespace input
{
	namespace
	{
		utils::hook::detour cl_char_event_hook;
		utils::hook::detour cl_key_event_hook;
		
		void cl_char_event_stub(const int local_client_num, const int key)
		{
			if(!game_console::console_char_event(local_client_num, key))
			{
				return;
			}
			
			cl_char_event_hook.invoke<void>(local_client_num, key);
		}

		void cl_key_event_stub(const int local_client_num, const int key, const int down)
		{
			if (!game_console::console_key_event(local_client_num, key, down))
			{
				return;
			}

			if(!server_list::sl_key_event(key, down))
			{
				return;
			}
			
			cl_key_event_hook.invoke<void>(local_client_num, key, down);
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

			cl_char_event_hook.create(SELECT_VALUE(0x14023CE50, 0x1402C2AE0), cl_char_event_stub);
			cl_key_event_hook.create(SELECT_VALUE(0x14023D070, 0x1402C2CE0), cl_key_event_stub);
		}
	};
}

REGISTER_MODULE(input::module)
