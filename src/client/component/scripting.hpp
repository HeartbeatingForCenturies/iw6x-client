#pragma once

namespace scripting
{
	extern std::unordered_map<std::string, std::unordered_map<std::string, const char*>> script_function_table;

	void on_shutdown(const std::function<void(int)>& callback);
}
