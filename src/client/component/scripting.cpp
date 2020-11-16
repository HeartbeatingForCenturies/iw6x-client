#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"
#include "utils/hook.hpp"

namespace scripting
{
	namespace
	{
		class script_value
		{
		public:
			game::VariableValue value{0, game::VAR_UNDEFINED};

			script_value(const game::VariableValue& value_)
			{
				this->assign(value_);
			}

			script_value(const script_value& other) noexcept
			{
				this->operator=(other);
			}

			script_value(script_value&& other) noexcept
			{
				this->operator=(std::move(other));
			}

			script_value& operator=(const script_value& other) noexcept
			{
				if (this != &other)
				{
					this->release();
					this->assign(other.value);
				}

				return *this;
			}

			script_value& operator=(script_value&& other) noexcept
			{
				if (this != &other)
				{
					this->operator=(other);
					other.release();
				}

				return *this;
			}

			~script_value()
			{
				this->release();
			}

		private:
			void assign(const game::VariableValue& value_)
			{
				this->value = value_;
				game::AddRefToValue(this->value.type, this->value.u);
			}

			void release()
			{
				if (this->value.type != game::VAR_UNDEFINED)
				{
					game::RemoveRefToValue(this->value.type, this->value.u);
					this->value.type = game::VAR_UNDEFINED;
				}
			}
		};

		struct event
		{
			std::string name;
			unsigned int entity_id;
			std::vector<script_value> arguments;
		};

		utils::hook::detour vm_notify_hook;

		void vm_notify_stub(const unsigned int notify_list_owner_id, const game::scr_string_t string_value,
		                    game::VariableValue* top)
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

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			vm_notify_hook.create(SELECT_VALUE(0x1403E29C0, 0x14043D9B0), vm_notify_stub);
		}
	};
}

REGISTER_COMPONENT(scripting::component)
