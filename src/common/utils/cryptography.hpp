#pragma once

#include <string>
#include <tomcrypt.h>

namespace utils::cryptography
{
	namespace ecc
	{
		class key final
		{
		public:
			key();
			~key();

			bool is_valid() const;

			ecc_key* get();

			std::string get_public_key() const;

			void set(const std::string& pub_key_buffer);

			void deserialize(const std::string& key);

			std::string serialize(int type = PK_PRIVATE) const;

			void free();

			bool operator==(key& key) const;

		private:
			ecc_key key_storage_{};
		};

		key generate_key(int bits);
		std::string sign_message(key key, const std::string& message);
		bool verify_message(key key, const std::string& message, const std::string& signature);
	}

	namespace rsa
	{
		std::string encrypt(const std::string& data, const std::string& hash, const std::string& key);
	}

	namespace des3
	{
		std::string encrypt(const std::string& data, const std::string& iv, const std::string& key);
		std::string decrypt(const std::string& data, const std::string& iv, const std::string& key);
	}

	namespace tiger
	{
		std::string compute(const std::string& data, bool hex = false);
		std::string compute(const uint8_t* data, size_t length, bool hex = false);
	}

	namespace sha1
	{
		std::string compute(const std::string& data, bool hex = false);
		std::string compute(const uint8_t* data, size_t length, bool hex = false);
	}

	namespace sha256
	{
		std::string compute(const std::string& data, bool hex = false);
		std::string compute(const uint8_t* data, size_t length, bool hex = false);
	}

	namespace sha512
	{
		std::string compute(const std::string& data, bool hex = false);
		std::string compute(const uint8_t* data, size_t length, bool hex = false);
	}

	namespace jenkins_one_at_a_time
	{
		unsigned int compute(const std::string& data);
		unsigned int compute(const char* key, size_t len);
	};

	namespace random
	{
		uint32_t get_integer();
		std::string get_challenge();
		void get_data(void* data, size_t size);
	}
}
