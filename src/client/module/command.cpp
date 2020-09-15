#include <std_include.hpp>
#include "command.hpp"

#include "game/game.hpp"
#include "utils/string.hpp"

std::unordered_map<std::string, std::function<void(command::params&)>> command::handlers;

int command::params::size()
{
    return game::native::Cmd_Argc();
}

const char* command::params::get(int index)
{
    return game::native::Cmd_Argv(index);
}

std::string command::params::join(int index)
{
    std::string result = {};

    for (int i = index; i < this->size(); i++)
    {
        if (i > index) result.append(" ");
        result.append(this->get(i));
    }
    return result;
}

void command::add_raw(const char* name, void(*callback)())
{
    game::native::Cmd_AddCommandInternal(name, callback, new game::native::cmd_function_s);
}

void command::add(const char* name, std::function<void(params&)> callback)
{
    std::string command = utils::string::to_lower(name);

    if (handlers.find(command) == handlers.end())
        add_raw(name, main_handler);

    handlers[command] = callback;
}

void command::execute(std::string input, bool sync)
{
    input.append("\n");

    if (sync)
    {
        game::native::Cmd_ExecuteSingleCommand(0, 0, input.data());
    }
    else
    {
        game::native::Cbuf_AddText(0, input.data());
    }
}

void command::main_handler()
{
    params params = {};

    std::string command = utils::string::to_lower(params[0]);
    if (handlers.find(command) != handlers.end())
    {
        handlers[command](params);
    }
}


command::command()
{
	
}

REGISTER_MODULE(command);
