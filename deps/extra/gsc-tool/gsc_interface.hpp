#pragma once
#undef ERROR
#undef IN
#undef TRUE
#undef FALSE

#undef far

#include <stdinc.hpp>
#include <iw6/iw6_pc.hpp>

namespace gsc
{
	extern std::unique_ptr<xsk::gsc::iw6_pc::context> cxt;
}
