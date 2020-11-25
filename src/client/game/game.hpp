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
			: sp_object_(reinterpret_cast<T*>(sp_address))
			  , mp_object_(reinterpret_cast<T*>(mp_address))
		{
		}

		operator T*() const
		{
			if (environment::is_sp())
			{
				return sp_object_;
			}

			return mp_object_;
		}

		T* operator->() const
		{
			return this->operator T*();
		}

	private:
		T* sp_object_;
		T* mp_object_;
	};

	int Cmd_Argc();
	const char* Cmd_Argv(int index);

	int SV_Cmd_Argc();
	const char* SV_Cmd_Argv(int index);
}

#include "symbols.hpp"
