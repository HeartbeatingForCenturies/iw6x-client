#include <std_include.hpp>
#include "loader/module_loader.hpp"

#include "game/game.hpp"
#include "utils/hook.hpp"

namespace scripting
{
	namespace
	{
		utils::hook::detour vm_notify_hook;
	
		void vm_notify_stub(unsigned int notifyListOwnerId, game::scr_string_t stringValue, game::VariableValue *top)
		{
			const auto* string = game::SL_ConvertToString(stringValue);
			if (string)
			{
				//printf("Event: %s\n", string);
			}
			
			vm_notify_hook.invoke<void>(notifyListOwnerId, stringValue, top);
		}
	}

	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			vm_notify_hook.create(SELECT_VALUE(0x1403E29C0, 0x14043D9B0), vm_notify_stub);
		}
	};
}

REGISTER_MODULE(scripting::module)
