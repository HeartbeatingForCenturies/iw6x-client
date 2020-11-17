#pragma once
#include "game/game.hpp"

namespace scripting
{
	class script_value;
	
	class entity final
	{
	public:
		entity();
		entity(unsigned int entity_id);

		entity(const entity& other);
		entity(entity&& other) noexcept;

		~entity();

		entity& operator=(const entity& other);
		entity& operator=(entity&& other) noexcept;

		void set(const std::string& field, const script_value& value) const;
		script_value get(const std::string& field) const;

		script_value call(const std::string& name, const std::vector<script_value>& arguments) const;

		unsigned int get_entity_id() const;
		game::scr_entref_t get_entity_reference() const;

	private:
		unsigned int entity_id_;

		void add() const;
		void release() const;
	};
}
