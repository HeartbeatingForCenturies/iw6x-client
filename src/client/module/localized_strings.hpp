#pragma once
#include "loader/module_loader.hpp"

class localized_strings final : public module_interface
{
public:
	static void override(const std::string& key, const std::string& value);

	void post_unpack() override;
};
