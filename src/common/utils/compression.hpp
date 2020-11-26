#pragma once

#include <string>

#define CHUNK 16384u

namespace utils::compression
{
	class zlib final
	{
	public:
		static std::string compress(const std::string& data);
		static std::string decompress(const std::string& data);
	};

#ifdef ENABLE_ZSTD
	class zstd final
	{
	public:
		static std::string compress(const std::string& data);
		static std::string decompress(const std::string& data);
	};
#endif
};
