#pragma once

namespace scripting
{
	extern std::unordered_map<std::string, std::unordered_map<std::string, const char*>> script_function_table;
	extern std::unordered_map<std::string, std::vector<std::pair<std::string, const char*>>> script_function_table_sort;

	void on_shutdown(const std::function<void(int)>& callback);

	std::optional<std::string> get_canonical_string(unsigned int id);
	std::string get_token_single(unsigned int id);
}
