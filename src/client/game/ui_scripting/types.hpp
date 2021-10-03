#pragma once
#include "game/game.hpp"
#include "script_value.hpp"

namespace ui_scripting
{
	class lightuserdata
	{
	public:
		lightuserdata(void*);
		void* ptr;
	};

	class userdata
	{
	public:
		userdata(void*);

		script_value get(const script_value& key) const;
		void set(const script_value& key, const script_value& value) const;
		arguments call(const std::string& name, const arguments& arguments) const;

		void* ptr;
	};

	class table
	{
	public:
		table();
		table(game::hks::HashTable* ptr_);

		script_value get(const script_value& key) const;
		void set(const script_value& key, const script_value& value) const;

		game::hks::HashTable* ptr;
	};

	class function
	{
	public:
		function(game::hks::cclosure*, game::hks::HksObjectType);

		arguments call(const arguments& arguments) const;

		game::hks::cclosure* ptr;
		game::hks::HksObjectType type;
	};
}
