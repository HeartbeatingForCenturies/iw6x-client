#pragma once
#include "loader/module_loader.hpp"

class command final : public module_interface
{
public:
	void post_unpack() override;

	class params
	{
	public:
		int size();
		const char* get(int index);
		std::string join(int index);
		const char* operator[](int index) { return this->get(index); }
	};

	class params_sv
	{
	public:
		int size();
		const char* get(int index);
		std::string join(int index);
		const char* operator[](int index) { return this->get(index); }
	};

	static void add_raw(const char* name, void (*callback)());
	static void add(const char* name, const std::function<void(params&)>& callback);
	static void add(const char* name, const std::function<void()>& callback);

	static void add_sv(const char* name, std::function<void(int, params_sv&)> callback);

	static void add_sp_commands();
	static void add_mp_commands();

private:
	static std::unordered_map<std::string, std::function<void(params&)>> handlers;
	static std::unordered_map<std::string, std::function<void(int, params_sv&)>> handlers_sv;

	static void main_handler();

	static void client_command(int clientNum, void* a2);
};
