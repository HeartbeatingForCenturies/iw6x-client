#include <std_include.hpp>
#include "loader/module_loader.hpp"

#include "game/game.hpp"
#include "utils/hook.hpp"

namespace scripting
{
	namespace
	{
		struct script_value
		{
			game::VariableValue value;

			script_value(const game::VariableValue& value_)
				: value(value_)
			{
				game::AddRefToValue(this->value.type, this->value.u);
			}

			~script_value()
			{
				game::RemoveRefToValue(this->value.type, this->value.u);
			}
		};

		struct event
		{
			std::string name;
			unsigned int entity_id;
			std::vector<script_value> arguments;
		};

		utils::hook::detour vm_notify_hook;
	
		void vm_notify_stub(const unsigned int notify_list_owner_id, const game::scr_string_t string_value, game::VariableValue *top)
		{
			const auto* string = game::SL_ConvertToString(string_value);
			if (string)
			{
				event e;
				e.name = string;
				e.entity_id = notify_list_owner_id;

				for (auto* value = top; value->type != game::VAR_PRECODEPOS; --value)
				{
					e.arguments.emplace_back(*value);
				}
			}
			
			vm_notify_hook.invoke<void>(notify_list_owner_id, string_value, top);
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
