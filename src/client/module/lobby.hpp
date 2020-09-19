#pragma once
#include "loader/module_loader.hpp"

class lobby final : public module
{
public:
	lobby();
	~lobby();

	static void remove(uint64_t lobby);
	static void set_data(uint64_t lobby, const std::string& key, const std::string& value);
	static std::string get_data(uint64_t lobby, const std::string& key);

	static void set_remote(uint64_t lobby, const std::string& key, const std::string& value);

private:
	static std::map<uint64_t, std::map<std::string, std::string>> data_;

	static void delete_lobby(const std::string& data);
	static void parse_lobby(const std::string& data);
};
