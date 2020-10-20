#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "party.hpp"

#include "command.hpp"
#include "network.hpp"

#include "steam/steam.hpp"

#include "utils/string.hpp"
#include "utils/info_string.hpp"
#include "utils/cryptography.hpp"

namespace party
{
	namespace
	{
		struct
		{
			game::netadr_s host{};
			std::string challenge{};
		} connect_state;

		void connect_to_party(const game::netadr_s& target, const std::string& mapname, const std::string& gametype)
		{
			if (game::environment::is_sp())
			{
				return;
			}

			// This fixes several crashes and impure client stuff
			game::Cmd_ExecuteSingleCommand(0, 0, "xblive_privatematch 1\n");

			// CL_ConnectFromParty
			char session_info[0x100] = {};
			reinterpret_cast<void(*)(int, char*, const game::netadr_s*, const char*, const char*)>(0x1402C5700)(
				0, session_info, &target, mapname.data(), gametype.data());
		}

		std::string get_dvar_string(const std::string& dvar)
		{
			auto* dvar_value = game::Dvar_FindVar(dvar.data());
			if (dvar_value && dvar_value->current.string)
			{
				return dvar_value->current.string;
			}

			return {};
		}

		int get_client_count()
		{
			auto count = 0;
			for (auto i = 0; i < *game::mp::svs_numclients; ++i)
			{
				if (game::mp::svs_clients[i].header.state >= 3)
				{
					++count;
				}
			}

			return count;
		}
	}

	void connect(const game::netadr_s& target)
	{
		if (game::environment::is_sp())
		{
			return;
		}

		connect_state.host = target;
		connect_state.challenge = utils::cryptography::random::get_challenge();

		network::send(target, "getInfo", connect_state.challenge);
	}

	class module final : public module_interface
	{
	public:
		void post_unpack() override
		{
			if (game::environment::is_sp())
			{
				return;
			}

			command::add("map", [](command::params& argument)
			{
				if (argument.size() != 2)
				{
					return;
				}

				game::SV_StartMapForParty(0, argument[1], false, false);
			});

			command::add("connect", [](command::params& argument)
			{
				if (argument.size() != 2)
				{
					return;
				}

				game::netadr_s target{};
				if (game::NET_StringToAdr(argument[1], &target))
				{
					connect(target);
				}
			});

			network::on("getInfo", [](const game::netadr_s& target, const std::string_view& data)
			{
				utils::info_string info{};
				info.set("challenge", std::string{data});
				info.set("gamename", "IW6");
				info.set("hostname", get_dvar_string("sv_hostname"));
				info.set("gametype", get_dvar_string("g_gametype"));
				info.set("xuid", utils::string::va("%llX", steam::SteamUser()->GetSteamID().bits));
				info.set("mapname", get_dvar_string("mapname"));
				info.set("isPrivate", get_dvar_string("g_password").empty() ? "0" : "1");
				info.set("clients", utils::string::va("%i", get_client_count()));
				info.set("sv_maxclients", utils::string::va("%i", *game::mp::svs_numclients));
				//info.set("protocol", utils::string::va("%i", PROTOCOL));
				//info.set("shortversion", SHORTVERSION);
				//info.set("hc", (Dvar::Var("g_hardcore").get<bool>() ? "1" : "0"));

				network::send(target, "infoResponse", info.build());
			});

			network::on("infoResponse", [](const game::netadr_s& target, const std::string_view& data)
			{
				if (connect_state.host != target)
				{
					printf("Connect response from stray host.\n");
					return;
				}

				utils::info_string info{data};

				if (info.get("challenge") != connect_state.challenge)
				{
					printf("Invalid challenge.\n");
					return;
				}

				const auto mapname = info.get("mapname");
				if (mapname.empty())
				{
					printf("Invalid map.\n");
					return;
				}

				const auto gametype = info.get("gametype");
				if (gametype.empty())
				{
					printf("Invalid gametype.\n");
					return;
				}

				connect_to_party(target, mapname, gametype);
			});
		}
	};
}

REGISTER_MODULE(party::module)
