#pragma once
#include "loader/module_loader.hpp"
#include "utils/concurrent_list.hpp"

class command final : public module
{	
public:
	command();

    class params
    {
    public:
        int size();
        const char* get(int index);
        std::string join(int index);
        const char* operator[](int index) { return this->get(index); }
    };

    static void add_raw(const char* name, void(*callback)());
    static void add(const char* name, std::function<void(params&)> callback);
    static void execute(std::string input, bool sync = false);


private:
    static std::unordered_map<std::string, std::function<void(params&)>> handlers;

    static void main_handler();

};
