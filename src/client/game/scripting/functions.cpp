#include <std_include.hpp>
#include "functions.hpp"

#include <utils/string.hpp>

#include <xsk/gsc/types.hpp>
#include <xsk/resolver.hpp>

namespace scripting
{
	namespace
	{
		int find_function_index(const std::string& name, const bool prefer_global)
		{
			const auto target = utils::string::to_lower(name);
			auto first = xsk::gsc::iw6::resolver::function_id;
			auto second = xsk::gsc::iw6::resolver::method_id;
			if (!prefer_global)
			{
				std::swap(first, second);
			}

			const auto first_res = first(target);
			if (first_res)
			{
				return first_res;
			}

			const auto second_res = second(target);
			if (second_res)
			{
				return second_res;
			}

			return -1;
		}

		script_function get_function_by_index(const std::uint32_t index)
		{
			static const auto function_table = SELECT_VALUE(0x144E1E6F0, 0x1446B77A0);
			static const auto method_table = SELECT_VALUE(0x144E1F9E0, 0x1446B8A90);

			if (index <= 0x1000)
			{
				return reinterpret_cast<script_function*>(function_table)[index - 1];
			}

			return reinterpret_cast<script_function*>(method_table)[index - 0x8000];
		}

		std::uint32_t parse_token_id(const std::string& name)
		{
			if (name.starts_with("_ID"))
			{
				return static_cast<std::uint32_t>(std::strtol(name.substr(3).data(), nullptr, 10));
			}

			return 0;
		}
	}

	std::string find_token(std::uint32_t id)
	{
		return xsk::gsc::iw6::resolver::token_name(static_cast<std::uint16_t>(id));
	}

	std::string find_token_single(std::uint32_t id)
	{
		return xsk::gsc::iw6::resolver::token_name(static_cast<std::uint16_t>(id));
	}

	unsigned int find_token_id(const std::string& name)
	{
		const auto id = xsk::gsc::iw6::resolver::token_id(name);
		if (id)
		{
			return id;
		}

		const auto parsed_id = parse_token_id(name);
		if (parsed_id)
		{
			return parsed_id;
		}

		return game::SL_GetCanonicalString(name.data());
	}

	script_function find_function(const std::string& name, const bool prefer_global)
	{
		const auto index = find_function_index(name, prefer_global);
		if (index < 0) return nullptr;

		return get_function_by_index(index);
	}
}
