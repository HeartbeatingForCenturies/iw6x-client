#include <std_include.hpp>
#include "string.hpp"
#include "cryptography.hpp"

/// http://www.opensource.apple.com/source/CommonCrypto/CommonCrypto-55010/Source/libtomcrypt/doc/libTomCryptDoc.pdf

namespace utils::cryptography
{
	namespace
	{
		void initialize_math()
		{
			static bool initialized = false;
			if (!initialized)
			{
				initialized = true;
				ltc_mp = ltm_desc;
			}
		}
	}

	ecc::key::key()
	{
		ZeroMemory(&this->key_storage_, sizeof(this->key_storage_));
	}

	ecc::key::~key()
	{
		this->free();
	}

	bool ecc::key::is_valid() const
	{
		return (!memory::is_set(&this->key_storage_, 0, sizeof(this->key_storage_)));
	}

	ecc_key* ecc::key::get()
	{
		return &this->key_storage_;
	}

	std::string ecc::key::get_public_key() const
	{
		uint8_t buffer[512] = {0};
		DWORD length = sizeof(buffer);

		if (ecc_ansi_x963_export(&this->key_storage_, buffer, &length) == CRYPT_OK)
		{
			return std::string(reinterpret_cast<char*>(buffer), length);
		}

		return {};
	}

	void ecc::key::set(const std::string& pub_key_buffer)
	{
		this->free();

		if (ecc_ansi_x963_import(reinterpret_cast<const uint8_t*>(pub_key_buffer.data()), ULONG(pub_key_buffer.size()),
		                         &this->key_storage_) != CRYPT_OK)
		{
			ZeroMemory(&this->key_storage_, sizeof(this->key_storage_));
		}
	}

	void ecc::key::deserialize(const std::string& key)
	{
		this->free();

		if (ecc_import(reinterpret_cast<const uint8_t*>(key.data()), ULONG(key.size()), &this->key_storage_) != CRYPT_OK
		)
		{
			ZeroMemory(&this->key_storage_, sizeof(this->key_storage_));
		}
	}

	std::string ecc::key::serialize(const int type) const
	{
		uint8_t buffer[4096] = {0};
		DWORD length = sizeof(buffer);

		if (ecc_export(buffer, &length, type, &this->key_storage_) == CRYPT_OK)
		{
			return std::string(reinterpret_cast<char*>(buffer), length);
		}

		return "";
	}

	void ecc::key::free()
	{
		if (this->is_valid())
		{
			ecc_free(&this->key_storage_);
		}

		ZeroMemory(&this->key_storage_, sizeof(this->key_storage_));
	}

	bool ecc::key::operator==(key& key) const
	{
		return (this->is_valid() && key.is_valid() && this->serialize(PK_PUBLIC) == key.serialize(PK_PUBLIC));
	}

	ecc::key ecc::generate_key(const int bits)
	{
		key key;

		initialize_math();
		register_prng(&sprng_desc);
		ecc_make_key(nullptr, find_prng("sprng"), bits / 8, key.get());

		return key;
	}

	std::string ecc::sign_message(key key, const std::string& message)
	{
		if (!key.is_valid()) return "";

		uint8_t buffer[512];
		DWORD length = sizeof(buffer);

		initialize_math();
		register_prng(&sprng_desc);
		ecc_sign_hash(reinterpret_cast<const uint8_t*>(message.data()), ULONG(message.size()), buffer, &length, nullptr,
		              find_prng("sprng"), key.get());

		return std::string(reinterpret_cast<char*>(buffer), length);
	}

	bool ecc::verify_message(key key, const std::string& message, const std::string& signature)
	{
		if (!key.is_valid()) return false;

		initialize_math();

		auto result = 0;
		return (ecc_verify_hash(reinterpret_cast<const uint8_t*>(signature.data()), ULONG(signature.size()),
		                        reinterpret_cast<const uint8_t*>(message.data()), ULONG(message.size()), &result,
		                        key.get()) == CRYPT_OK && result != 0);
	}

	std::string rsa::encrypt(const std::string& data, const std::string& hash, const std::string& key)
	{
		initialize();

		const auto prng_id = find_prng("yarrow");

		rsa_key new_key;
		rsa_import(PBYTE(key.data()), ULONG(key.size()), &new_key);

		prng_state yarrow;
		rng_make_prng(128, prng_id, &yarrow, nullptr);

		unsigned char buffer[0x80];
		unsigned long length = sizeof(buffer);

		const auto rsa_result = rsa_encrypt_key( //
			PBYTE(data.data()), //
			ULONG(data.size()), //
			buffer, //
			&length, //
			PBYTE(hash.data()), //
			ULONG(hash.size()), //
			&yarrow, //
			prng_id, //
			find_hash("sha1"), //
			&new_key);

		rsa_free(&new_key);

		if (rsa_result == CRYPT_OK)
		{
			return std::string(PCHAR(buffer), length);
		}

		return {};
	}

	void rsa::initialize()
	{
		static auto initialized = false;
		if (initialized) return;
		initialized = true;

		initialize_math();
		register_hash(&sha1_desc);
		register_prng(&yarrow_desc);
	}

	std::string des3::encrypt(const std::string& data, const std::string& iv, const std::string& key)
	{
		initialize();

		std::string enc_data;
		enc_data.resize(data.size());

		symmetric_CBC cbc;
		const auto des3 = find_cipher("3des");

		cbc_start(des3, reinterpret_cast<const uint8_t*>(iv.data()), reinterpret_cast<const uint8_t*>(key.data()),
		          static_cast<int>(key.size()), 0, &cbc);
		cbc_encrypt(reinterpret_cast<const uint8_t*>(data.data()),
		            reinterpret_cast<uint8_t*>(const_cast<char*>(enc_data.data())), ULONG(data.size()), &cbc);
		cbc_done(&cbc);

		return enc_data;
	}

	std::string des3::decrypt(const std::string& data, const std::string& iv, const std::string& key)
	{
		initialize();

		std::string dec_data;
		dec_data.resize(data.size());

		symmetric_CBC cbc;
		const auto des3 = find_cipher("3des");

		cbc_start(des3, reinterpret_cast<const uint8_t*>(iv.data()), reinterpret_cast<const uint8_t*>(key.data()),
		          static_cast<int>(key.size()), 0, &cbc);
		cbc_decrypt(reinterpret_cast<const uint8_t*>(data.data()),
		            reinterpret_cast<uint8_t*>(const_cast<char*>(dec_data.data())), ULONG(data.size()), &cbc);
		cbc_done(&cbc);

		return dec_data;
	}

	void des3::initialize()
	{
		static auto initialized = false;
		if (initialized) return;
		initialized = true;

		register_cipher(&des3_desc);
	}

	std::string tiger::compute(const std::string& data, const bool hex)
	{
		return compute(reinterpret_cast<const uint8_t*>(data.data()), data.size(), hex);
	}

	std::string tiger::compute(const uint8_t* data, const size_t length, const bool hex)
	{
		uint8_t buffer[24] = {0};

		hash_state state;
		tiger_init(&state);
		tiger_process(&state, data, ULONG(length));
		tiger_done(&state, buffer);

		std::string hash(reinterpret_cast<char*>(buffer), sizeof(buffer));
		if (!hex) return hash;

		return string::dump_hex(hash, "");
	}

	std::string sha1::compute(const std::string& data, const bool hex)
	{
		return compute(reinterpret_cast<const uint8_t*>(data.data()), data.size(), hex);
	}

	std::string sha1::compute(const uint8_t* data, const size_t length, const bool hex)
	{
		uint8_t buffer[20] = {0};

		hash_state state;
		sha1_init(&state);
		sha1_process(&state, data, ULONG(length));
		sha1_done(&state, buffer);

		std::string hash(reinterpret_cast<char*>(buffer), sizeof(buffer));
		if (!hex) return hash;

		return string::dump_hex(hash, "");
	}

	std::string sha256::compute(const std::string& data, const bool hex)
	{
		return compute(reinterpret_cast<const uint8_t*>(data.data()), data.size(), hex);
	}

	std::string sha256::compute(const uint8_t* data, const size_t length, const bool hex)
	{
		uint8_t buffer[32] = {0};

		hash_state state;
		sha256_init(&state);
		sha256_process(&state, data, ULONG(length));
		sha256_done(&state, buffer);

		std::string hash(reinterpret_cast<char*>(buffer), sizeof(buffer));
		if (!hex) return hash;

		return string::dump_hex(hash, "");
	}

	std::string sha512::compute(const std::string& data, const bool hex)
	{
		return compute(reinterpret_cast<const uint8_t*>(data.data()), data.size(), hex);
	}

	std::string sha512::compute(const uint8_t* data, const size_t length, const bool hex)
	{
		uint8_t buffer[64] = {0};

		hash_state state;
		sha512_init(&state);
		sha512_process(&state, data, ULONG(length));
		sha512_done(&state, buffer);

		std::string hash(reinterpret_cast<char*>(buffer), sizeof(buffer));
		if (!hex) return hash;

		return string::dump_hex(hash, "");
	}

	unsigned int jenkins_one_at_a_time::compute(const std::string& data)
	{
		return compute(data.data(), data.size());
	}

	unsigned int jenkins_one_at_a_time::compute(const char* key, const size_t len)
	{
		unsigned int hash, i;
		for (hash = i = 0; i < len; ++i)
		{
			hash += key[i];
			hash += (hash << 10);
			hash ^= (hash >> 6);
		}
		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);
		return hash;
	}

	uint32_t random::get_integer()
	{
		uint32_t result;
		random::get_data(&result, sizeof(result));
		return result;
	}

	std::string random::get_challenge()
	{
		std::string result;
		result.resize(sizeof(uint32_t));
		random::get_data(result.data(), result.size());
		return string::dump_hex(result, "");
	}

	void random::get_data(void* data, const size_t size)
	{
		fortuna_read(static_cast<unsigned char*>(data), static_cast<unsigned long>(size), random::get_prng_state());
	}

	prng_state* random::get_prng_state()
	{
		static prng_state* state_ref = []()
		{
			static prng_state state;

			initialize_math();
			register_prng(&fortuna_desc);
			rng_make_prng(128, find_prng("fortuna"), &state, nullptr);

			int i[4]; // uninitialized data
			auto i_ptr = &i;
			fortuna_add_entropy(reinterpret_cast<unsigned char*>(&i), sizeof(i), &state);
			fortuna_add_entropy(reinterpret_cast<unsigned char*>(&i_ptr), sizeof(i_ptr), &state);

			auto t = time(nullptr);
			fortuna_add_entropy(reinterpret_cast<unsigned char*>(&t), sizeof(t), &state);

			return &state;
		}();

		fortuna_ready(state_ref);
		return state_ref;
	}
}
