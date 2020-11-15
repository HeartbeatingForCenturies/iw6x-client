#pragma once

#include "structs.hpp"
#include "launcher/launcher.hpp"

#define SELECT_VALUE(sp, mp) (game::environment::is_sp() ? (sp) : (mp))

#define SERVER_CD_KEY "IW6x-CD-Key"

namespace game
{
	namespace environment
	{
		launcher::mode get_mode();

		bool is_mp();
		bool is_sp();
		bool is_dedi();

		void set_mode(launcher::mode mode);
	}

	template <typename T>
	class Symbol
	{
	public:
		Symbol(const size_t sp_address, const size_t mp_address)
			: sp_address_(sp_address)
			  , mp_address_(mp_address)
		{
		}

		[[nodiscard]] size_t get() const
		{
			if (environment::is_sp())
			{
				return sp_address_;
			}

			return mp_address_;
		}

		operator T*() const
		{
			return reinterpret_cast<T*>(get());
		}

		T* operator->() const
		{
			return this->operator T*();
		}

	private:
		size_t sp_address_{0};
		size_t mp_address_{0};
	};

	int Cmd_Argc();
	const char* Cmd_Argv(int index);

	int SV_Cmd_Argc();
	const char* SV_Cmd_Argv(int index);
}

#include "symbols.hpp"
