#pragma once
#include "game/game.hpp"

namespace scripting
{
	class vector final
	{
	public:
		vector() = default;
		vector(const float* value);
		vector(const game::vec3_t& value);
		vector(float x, float y, float z);

		 operator game::vec3_t&();
		 operator const game::vec3_t&() const;
		
		game::vec_t& operator[](size_t i);
		const game::vec_t& operator[](size_t i) const;

	private:
		game::vec3_t value_{0};
	};
}
