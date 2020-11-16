#include <string>
#include <sstream>
#include <fstream>
#include <regex>
#include <functional>
#include <map>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Don't ask...
#define main lululul
#define private protected
#include "../../deps/msvc-demangler/MicrosoftDemangle.cpp"
#undef private
#undef main

class MyDemangler : public Demangler
{
public:
	using Demangler::Demangler;

	std::string get_name()
	{
		write_name(symbol);
		return os.str();
	}
};

std::string read_file(const std::string& file)
{
	std::ifstream t(file);
	return std::string((std::istreambuf_iterator<char>(t)),
	                   std::istreambuf_iterator<char>());
}

std::string read_link_map()
{
	return read_file("iw6_ds.map");
}

void foreach_linker_line(const std::string& linker_map, const std::function<void(const std::string&)>& callback)
{
	std::string line{};
	std::stringstream ss(linker_map);

	while (std::getline(ss, line, '\n'))
	{
		if (!line.empty() && line.back() == '\r')
		{
			line.pop_back();
		}

		callback(line);
	}
}

std::unordered_map<uint64_t, std::string> parse_linker_functions(const std::string& linker_map)
{
	std::unordered_map<uint64_t, std::string> result{};
	std::regex function_matcher{R"(\s0001:[0-9a-f]+\s+([^ ]+) ([0-9a-f]+) f\s+.*)"};

	foreach_linker_line(linker_map, [&](const std::string& line)
	{
		std::smatch match;
		if (std::regex_match(line, match, function_matcher))
		{
			auto function = match[1].str();
			const auto address = strtoll(match[2].str().data(), nullptr, 16);
			result[address] = std::move(function);
		}
	});

	return result;
}

std::unordered_map<uint64_t, std::string> get_function_map()
{
	const auto linker_map = read_link_map();
	return parse_linker_functions(linker_map);
}

void load_dedi()
{
	if (!LoadLibraryA("iw6_ds.exe"))
	{
		std::abort();
	}
}

std::map<unsigned int, uint64_t> map_functions_with_table(const size_t table, const size_t size,
                                                          const unsigned offset)
{
	std::map<unsigned int, uint64_t> function_map{};

	for (size_t i = 0; i < size; ++i)
	{
		const auto address = reinterpret_cast<uint64_t*>(table)[i];
		if (address)
		{
			function_map[static_cast<unsigned>(i) + offset] = address;
		}
	}

	return function_map;
}

void initialize_function_table(const size_t table)
{
	int res;
	const char* name;
	reinterpret_cast<void(*)(const char**, int*)>(table)(&name, &res);
}

std::string demangle_name(const std::string& name)
{
	MyDemangler demangler(name);
	demangler.parse();
	return demangler.get_name();
}

std::map<std::string, unsigned> map_name_to_function(const std::map<unsigned int, uint64_t>& function_map,
                                                     const std::unordered_map<uint64_t, std::string>& name_map)
{
	std::map<std::string, unsigned> name_index_map{};

	for (const auto& func : function_map)
	{
		const auto name = name_map.find(func.second);
		if (name == name_map.end())
		{
			// Wat?
			continue;
		}

		const auto demangled_name = demangle_name(name->second);
		name_index_map[demangled_name] = func.first;

		printf("%X: %s\n", func.first, demangled_name.data());
	}

	return name_index_map;
}

int main()
{
	load_dedi();

	initialize_function_table(0x14015DE40); // Scr_GetMethod
	initialize_function_table(0x14015DD20); // Scr_GetFunction

	const auto methods = map_functions_with_table(0x141BA5060, 1070, 0x8000); // methods
	const auto functions = map_functions_with_table(0x141BA3D70, 606, 0); // functions

	const auto function_name_map = get_function_map();

	const auto method_map = map_name_to_function(methods, function_name_map);
	const auto function_map = map_name_to_function(functions, function_name_map);

	return 0;
}
