#include <std_include.hpp>
#include "safe_execution.hpp"

#pragma warning(push)
#pragma warning(disable: 4611)

namespace scripting::safe_execution
{
	bool has_failed(jmp_buf& buffer)
	{
		if (setjmp(buffer))
		{
			return true;
		}

		return false;
	}

	bool call(const script_function function, const game::scr_entref_t& entref)
	{
		*game::g_script_error_level += 1;
		if (has_failed(game::g_script_error[*game::g_script_error_level]))
		{
			*game::g_script_error_level -= 1;
			return false;
		}

		function(entref);

		*game::g_script_error_level -= 1;
		return true;
	}

	bool set_entity_field(const game::scr_entref_t& entref, const int offset)
	{
		*game::g_script_error_level += 1;
		if (has_failed(game::g_script_error[*game::g_script_error_level]))
		{
			*game::g_script_error_level -= 1;
			return false;
		}

		game::Scr_SetObjectField(entref.classnum, entref.entnum, offset);

		*game::g_script_error_level -= 1;
		return true;
	}

	bool get_entity_field(const game::scr_entref_t& entref, const int offset, game::VariableValue* value)
	{
		*game::g_script_error_level += 1;
		if (has_failed(game::g_script_error[*game::g_script_error_level]))
		{
			value->type = game::SCRIPT_NONE;
			value->u.intValue = 0;
			*game::g_script_error_level -= 1;
			return false;
		}

		game::GetEntityFieldValue(value, entref.classnum, entref.entnum, offset);

		*game::g_script_error_level -= 1;
		return true;
	}
}

#pragma warning(pop)
