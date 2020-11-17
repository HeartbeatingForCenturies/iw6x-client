#include <std_include.hpp>
#include "execution.hpp"
#include "safe_execution.hpp"
#include "stack_isolation.hpp"

#include <utils/string.hpp>

namespace scripting
{
	extern std::unordered_map<std::string, unsigned> method_map;
	extern std::unordered_map<std::string, unsigned> function_map;

	namespace
	{
		std::unordered_map<std::string, unsigned> lowercase_map(
			const std::unordered_map<std::string, unsigned>& old_map)
		{
			std::unordered_map<std::string, unsigned> new_map{};
			for (auto& entry : old_map)
			{
				new_map[utils::string::to_lower(entry.first)] = entry.second;
			}

			return new_map;
		}

		const std::unordered_map<std::string, unsigned>& get_methods()
		{
			static auto methods = lowercase_map(method_map);
			return methods;
		}

		const std::unordered_map<std::string, unsigned>& get_functions()
		{
			static auto function = lowercase_map(function_map);
			return function;
		}

		int find_function_index(const std::string& name, const bool prefer_global)
		{
			const auto target = utils::string::to_lower(name);

			const auto& primary_map = prefer_global
				                          ? get_functions()
				                          : get_methods();
			const auto& secondary_map = !prefer_global
				                            ? get_functions()
				                            : get_methods();

			auto function_entry = primary_map.find(target);
			if (function_entry != primary_map.end())
			{
				return function_entry->second;
			}

			function_entry = secondary_map.find(target);
			if (function_entry != secondary_map.end())
			{
				return function_entry->second;
			}

			return -1;
		}

		game::VariableValue* allocate_argument()
		{
			game::VariableValue* value_ptr = ++game::scr_VmPub->top;
			++game::scr_VmPub->inparamcount;
			return value_ptr;
		}

		void push_value(const script_value& value)
		{
			auto* value_ptr = allocate_argument();
			*value_ptr = value.get_raw();

			game::AddRefToValue(value_ptr->type, value_ptr->u);
		}

		// TODO: Finish
		void notify(const entity& entity, const std::string& event)
		{
			const auto event_id = game::SL_GetString(event.data(), 0);
			game::Scr_NotifyId(entity.get_entity_id(), event_id, game::scr_VmPub->inparamcount);
		}

		int get_field_id(const int classnum, const std::string& field)
		{
			const auto field_name = utils::string::to_lower(field);
			const auto class_id = game::g_classMap[classnum].id;
			const auto field_str = game::SL_GetString(field_name.data(), 1);
			const auto _ = gsl::finally([field_str]()
			{
				game::RemoveRefToValue(game::SCRIPT_STRING, {int(field_str)});
			});

			const auto offset = game::FindVariable(class_id, field_str);
			if (offset)
			{
				const auto index = 3 * (offset + 0xC800 * (class_id & 1));
				const auto id = PINT64(SELECT_VALUE(0x145359F80, 0x144AF3080))[index];
				return static_cast<int>(id);
			}

			return -1;
		}

		script_value get_return_value()
		{
			if (game::scr_VmPub->inparamcount == 0)
			{
				return {};
			}

			game::Scr_ClearOutParams();
			game::scr_VmPub->outparamcount = game::scr_VmPub->inparamcount;
			game::scr_VmPub->inparamcount = 0;

			return script_value(game::scr_VmPub->top[1 - game::scr_VmPub->outparamcount]);
		}

		safe_execution::script_function get_function_by_index(const unsigned index)
		{
			if (index < 0x25D)
			{
				return reinterpret_cast<safe_execution::script_function*>(SELECT_VALUE(0x144E1E6F0, 0x1446B77A0))[index
				];
			}

			return reinterpret_cast<safe_execution::script_function*>(SELECT_VALUE(0x144E1F9E0, 0x1446B8A90))[index -
				0x8000];
		}
	}

	script_value call_function(const std::string& name, const entity& entity,
	                           const std::vector<script_value>& arguments)
	{
		const auto entref = entity.get_entity_reference();

		const auto is_method_call = *reinterpret_cast<const int*>(&entref) != -1;
		const auto index = find_function_index(name, !is_method_call);
		if (index < 0)
		{
			return {};
		}

		const auto function = get_function_by_index(index);
		if (!function)
		{
			return {};
		}

		stack_isolation _;

		for (auto i = arguments.rbegin(); i != arguments.rend(); ++i)
		{
			push_value(*i);
		}

		game::scr_VmPub->outparamcount = game::scr_VmPub->inparamcount;
		game::scr_VmPub->inparamcount = 0;

		if (!safe_execution::call(function, entref))
		{
			throw std::runtime_error("Error executing function");
		}

		return get_return_value();
	}

	script_value call_function(const std::string& name, const std::vector<script_value>& arguments)
	{
		return call_function(name, entity(), arguments);
	}

	void set_entity_field(const entity& entity, const std::string& field, const script_value& value)
	{
		const auto entref = entity.get_entity_reference();
		const int id = get_field_id(entref.classnum, field);

		if (id != -1)
		{
			stack_isolation _;
			push_value(value);

			game::scr_VmPub->outparamcount = game::scr_VmPub->inparamcount;
			game::scr_VmPub->inparamcount = 0;

			if (!safe_execution::set_entity_field(entref, id))
			{
				throw std::runtime_error("Failed to set value for field '" + field + "'");
			}
		}
		else
		{
			// Read custom fields
		}
	}

	script_value get_entity_field(const entity& entity, const std::string& field)
	{
		const auto entref = entity.get_entity_reference();
		const auto id = get_field_id(entref.classnum, field);

		if (id != -1)
		{
			stack_isolation _;

			game::VariableValue value{};
			if (!safe_execution::get_entity_field(entref, id, &value))
			{
				throw std::runtime_error("Failed to get value for field '" + field + "'");
			}

			const auto __ = gsl::finally([value]()
			{
				game::RemoveRefToValue(value.type, value.u);
			});

			return value;
		}
		else
		{
			// Add custom fields
		}

		return {};
	}
}
