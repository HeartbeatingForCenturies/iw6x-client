#pragma once
#include <string>

namespace utils::toast
{
	bool show(const std::string& title, const std::string& text);
	bool show(const std::string& title, const std::string& text, const std::string& image);
}
