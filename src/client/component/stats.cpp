#include <std_include.hpp>
#include <utils/hook.hpp>
#include <utils/string.hpp>
#include "loader/component_loader.hpp"

#include "game/game.hpp"

#include "command.hpp"
#include "console.hpp"

namespace stats
{
	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			if (!game::environment::is_mp())
			{
				return;
			}

			command::add("setReservedPlayerDataInt", [](const command::params& params)
			{
				const std::vector<const char*> reserved_lookup_strings
				{
					//EXTINCTION MODE RESERVED LOOKUP STRINGS
					"extinction_purchase_flags",
					"extinction_tokens",
					"extinction_purchase_flags",
					"extinction_crafted_flags",
					"upgrades_enabled_flags",
					"relics_enabled_flags",
					"bonus_pool_size",
					"bonus_pool_deadline",
					//MULTIPLAYER
					"prestigeLevel",
					//COMMON
					"mp_announcer_type"
				};

				//Also checks if it's a valid lookup string otherwise it crashes the client.
				if (params.size() < 3 &&
					std::find(reserved_lookup_strings.begin(), reserved_lookup_strings.end(), params.get(1))
					== reserved_lookup_strings.end())
				{
					console::info("usage: setReservedPlayerData <lookup_string>, <value>\n");
					return;
				}

				const int controller_index{ 0 };
				const auto persistent_data_buffer{ game::LiveStorage_GetPersistentDataBuffer(controller_index) };
				const char* first_param{ params.get(1) };

				enum game::StatsGroup statsgroup { game::STATSGROUP_COOP };

				if (!strcmp(first_param, "prestigeLevel"))
				{
					statsgroup = game::STATSGROUP_RANKED;
				}
				else if (!strcmp(first_param, "mp_announcer_type"))
				{
					statsgroup = game::STATSGROUP_COMMON;
				}

				auto value = atoi(params.get(2));
				game::LiveStorage_PlayerDataSetReservedInt(persistent_data_buffer, first_param, value, 0, statsgroup);
			});

			command::add("getReservedPlayerDataInt", [](const command::params& params)
			{
				const std::vector<const char*> reserved_lookup_strings
				{
					//EXTINCTION MODE RESERVED LOOKUP STRINGS
					"extinction_purchase_flags",
					"extinction_tokens",
					"extinction_purchase_flags",
					"extinction_crafted_flags",
					"upgrades_enabled_flags",
					"relics_enabled_flags",
					"bonus_pool_size",
					"bonus_pool_deadline",
					//MULTIPLAYER
					"prestigeLevel",
					//COMMON
					"mp_announcer_type"
				};

				//Also checks if it's a valid lookup string otherwise it crashes the client.
				if (params.size() < 2 && 
					std::find(reserved_lookup_strings.begin(), reserved_lookup_strings.end(), params.get(1)) 
					== reserved_lookup_strings.end())
				{
					console::info("usage: getReservedPlayerData <lookup_string>\n");
					return;
				}

				const int controller_index{ 0 };
				const auto persistent_data_buffer{ game::LiveStorage_GetPersistentDataBuffer(controller_index) };
				const char* first_param{ params.get(1) };

				enum game::StatsGroup statsgroup { game::STATSGROUP_COOP };

				if (!strcmp(first_param, "prestigeLevel"))
				{
					statsgroup = game::STATSGROUP_RANKED;
				}
				else if (!strcmp(first_param, "mp_announcer_type"))
				{
					statsgroup = game::STATSGROUP_COMMON;
				}

				const auto result{ game::LiveStorage_PlayerDataGetReservedInt(persistent_data_buffer, first_param, statsgroup) };
				console::info("%d\n", result);
			});

			command::add("setPlayerDataInt", [](const command::params& params)
			{
				if (params.size() < 3)
				{
					console::info("usage: setPlayerDataInt <lookup_string>, ... , <lookup_string>, <value>\n");
					return;
				}

				const int controller_index{ 0 };
				const auto asset{ game::StructuredDataDef_GetAsset("mp/playerdata.def", 0x93FCu) };
				const char* first_param{ params.get(1) };

				bool found_param{ false };
				std::vector<game::scr_string_t> lookup_strings_vector{};
				enum game::StatsGroup statsgroup{ game::STATSGROUP_RANKED };
				
				for (int i = 0; i < asset->structs[23].propertyCount; ++i)
				{
					if (!strcmp(game::SL_ConvertToString(asset->structs[23].properties[i].name), first_param))
					{
						statsgroup = game::STATSGROUP_COMMON;
						found_param = true;
						break;
					}
				}

				//no need to search it again if we already found it at the common strings
				if (!found_param)
				{
					for (int i = 0; i < asset->structs[22].propertyCount; ++i)
					{
						if (!strcmp(game::SL_ConvertToString(asset->structs[22].properties[i].name), first_param))
						{
							statsgroup = game::STATSGROUP_COOP;
							break;
						}
					}
				}

				for (int i = 1; i < params.size() - 1; ++i)
				{
					if (utils::string::is_numeric(params.get(i)))
					{
						lookup_strings_vector.push_back(game::SL_GetStringForInt(atoi(params.get(i))));
					}
					else
					{
						lookup_strings_vector.push_back(game::SL_FindString(params.get(i)));
					}
				}

				const auto value = atoi(params.get(params.size() - 1));
				game::LiveStorage_PlayerDataSetIntByNameArray(
					controller_index, &lookup_strings_vector[0], params.size() - 2, value, statsgroup);

				//This is necessary for the stats to stick after closing the game
				auto* stat_write_needed = reinterpret_cast<bool*>(0x1445A7C72);
				if (stat_write_needed)
				{
					*stat_write_needed = true;
				}
			});

			command::add("getPlayerDataInt", [](const command::params& params)
			{
				if (params.size() < 2)
				{
					console::info("usage: getPlayerDataInt <lookup_string>\n");
					return;
				}

				const int controller_index{ 0 };
				const auto asset{ game::StructuredDataDef_GetAsset("mp/playerdata.def", 0x93FCu) };
				const char* first_param{ params.get(1) };

				bool found_param{ false };
				std::vector<game::scr_string_t> lookup_strings_vector{};
				enum game::StatsGroup statsgroup{ game::STATSGROUP_RANKED };

				for (int i = 0; i < asset->structs[23].propertyCount; ++i)
				{
					if (!strcmp(game::SL_ConvertToString(asset->structs[23].properties[i].name), first_param))
					{
						statsgroup = game::STATSGROUP_COMMON;
						found_param = true;
						break;
					}
				}

				//no need to search it again if we already found it at the common strings
				if (!found_param)
				{
					for (int i = 0; i < asset->structs[22].propertyCount; ++i)
					{
						if (!strcmp(game::SL_ConvertToString(asset->structs[22].properties[i].name), first_param))
						{
							statsgroup = game::STATSGROUP_COOP;
							break;
						}
					}
				}

				for (int i = 1; i < params.size(); ++i)
				{
					if (utils::string::is_numeric(params.get(i)))
					{
						lookup_strings_vector.push_back(game::SL_GetStringForInt(atoi(params.get(i))));
					}
					else
					{
						lookup_strings_vector.push_back(game::SL_FindString(params.get(i)));
					}
				}

				const auto result = game::LiveStorage_PlayerDataGetIntByNameArray(
						controller_index, &lookup_strings_vector[0], params.size() - 1, statsgroup);

				console::info("%d\n", result);
			});

			command::add("unlockstats", []()
			{
				game::scr_string_t nav_strings[5]{};
				const int controller_index{ 0 };
				const auto persistent_data_buffer{ game::LiveStorage_GetPersistentDataBuffer(controller_index) };

				/**
				* EXTINCTION
				**/

				nav_strings[0] = game::SL_FindString("alienPlayerStats");
				nav_strings[1] = game::SL_FindString("prestige");
				game::LiveStorage_PlayerDataSetIntByNameArray(
					controller_index, nav_strings, 2, 25, game::STATSGROUP_COOP);

				nav_strings[1] = game::SL_FindString("experience");
				game::LiveStorage_PlayerDataSetIntByNameArray(
					controller_index, nav_strings, 2, 1845000, game::STATSGROUP_COOP);

				nav_strings[1] = game::SL_FindString("rank"); //Not sure if necessary 
				game::LiveStorage_PlayerDataSetIntByNameArray(
					controller_index, nav_strings, 2, 31, game::STATSGROUP_COOP);

				nav_strings[1] = game::SL_FindString("revives");
				game::LiveStorage_PlayerDataSetIntByNameArray(
					controller_index, nav_strings, 2, 100, game::STATSGROUP_COOP);

				nav_strings[1] = game::SL_FindString("kills");
				game::LiveStorage_PlayerDataSetIntByNameArray(
					controller_index, nav_strings, 2, 10000, game::STATSGROUP_COOP);

				nav_strings[1] = game::SL_FindString("escaped");
				game::LiveStorage_PlayerDataSetIntByNameArray(
					controller_index, nav_strings, 2, 10, game::STATSGROUP_COOP);

				// Lazy IW devs
				// Unlocks background with the challenges "Beat any Extinction map with x Relic(s) Active"
				nav_strings[1] = game::SL_FindString("headShots");
				game::LiveStorage_PlayerDataSetIntByNameArray(
					controller_index, nav_strings, 2, 5, game::STATSGROUP_COOP);

				game::LiveStorage_PlayerDataSetReservedInt(
					persistent_data_buffer, "extinction_purchase_flags", -1, 0, game::STATSGROUP_COOP);
				game::LiveStorage_PlayerDataSetReservedInt(
					persistent_data_buffer, "extinction_tokens", 5000, 0, game::STATSGROUP_COOP);

				/**
				* MULTIPLAYER
				**/

				const int amount_squad_members{ 10 };
				const int amount_regular_loadouts{ 6 };
				const game::StringTable* squad_unlocktable{ nullptr };
				game::StringTable_GetAsset("mp/squadunlocktable.csv", &squad_unlocktable);

				for (int i = 0; i < amount_squad_members; ++i)
				{
					//select specific squadmember
					nav_strings[0] = game::SL_FindString("squadMembers");
					nav_strings[1] = game::SL_GetStringForInt(i);

					//unlocks squadmember
					nav_strings[2] = game::SL_FindString("inUse");
					game::LiveStorage_PlayerDataSetIntByNameArray(
						controller_index, nav_strings, 3, 1, game::STATSGROUP_RANKED);

					//choose a default loadout otherwise unlocking loadouts won't work
					nav_strings[2] = game::SL_FindString("defaultSet");
					game::LiveStorage_PlayerDataSetIntByNameArray(
						controller_index, nav_strings, 3, 1, game::STATSGROUP_RANKED);

					//set max xp squad member
					nav_strings[2] = game::SL_FindString("squadMemXP");
					game::LiveStorage_PlayerDataSetIntByNameArray(
						controller_index, nav_strings, 3, 1230080, game::STATSGROUP_RANKED);

					//unlocks loadout
					nav_strings[2] = game::SL_FindString("loadouts");
					nav_strings[4] = game::SL_FindString("inUse");

					for (int j = 0; j < amount_regular_loadouts; ++j)
					{
						nav_strings[3] = game::SL_GetStringForInt(j);
						game::LiveStorage_PlayerDataSetIntByNameArray(
							controller_index, nav_strings, 5, 2, game::STATSGROUP_RANKED);
					}

					//unlocks all weapons, reticles and killstreaks 
					nav_strings[2] = game::SL_FindString("challengeState");
					if (squad_unlocktable)
					{
						for (int j = 0; j < squad_unlocktable->rowCount; ++j)
						{
							auto unlock_item = game::StringTable_GetColumnValueForRow(squad_unlocktable, j, 3);

							if (unlock_item[0] == '\0')
								continue;

							nav_strings[3] = game::SL_FindString(unlock_item);
							game::LiveStorage_PlayerDataSetIntByNameArray(
								controller_index, nav_strings, 4, 2, game::STATSGROUP_RANKED);
						}
					}
				}

				//not the correct way to set each squadmembers prestige
				//but i couldn't figure it out how it's done.
				for (int i = 0; i < amount_squad_members; ++i)
				{
					utils::hook::set<int>(0x1445A2B20 + (i * 4), i);
				}

				const game::StringTable* unlock_table{ nullptr };
				game::StringTable_GetAsset("mp/unlocktable.csv", &unlock_table);

				if (unlock_table)
				{
					//squad purchases challenge is defined at line 112 and ends at 152.
					//it's defined 4 times each hence the jumps of 4
					for (int i = 112; i < 152; i += 4)
					{
						// Register squad purchases for calling cards
						auto squad_member_purchase = game::StringTable_GetColumnValueForRow(unlock_table, i, 3);

						nav_strings[0] = game::SL_FindString("challengeState");
						nav_strings[1] = game::SL_FindString(squad_member_purchase);

						game::LiveStorage_PlayerDataSetIntByNameArray(
							controller_index, nav_strings, 2, 2, game::STATSGROUP_RANKED);

						nav_strings[0] = game::SL_FindString("challengeProgress");
						game::LiveStorage_PlayerDataSetIntByNameArray(
							controller_index, nav_strings, 2, 1, game::STATSGROUP_RANKED);
					}
				}

				//set the players prestige to 10th prestige
				game::LiveStorage_PlayerDataSetReservedInt(
					persistent_data_buffer, "prestigeLevel", 10, 0, game::STATSGROUP_RANKED);

				//gives unlock points but not really necassary since everything is already unlocked
				game::LiveStorage_PlayerDataSetIntByName(
					controller_index, game::SL_FindString("unlockPoints"), 5000, game::StatsGroup::STATSGROUP_RANKED);

				/**
				* UNLOCKING ALL CHALLENGES
				**/

				//sets every challenge to completed
				const game::StringTable* challenges_table{ nullptr };
				game::StringTable_GetAsset("mp/allchallengestable.csv", &challenges_table);

				if (challenges_table)
				{
					for (int i = 0; i < challenges_table->rowCount; ++i)
					{
						// Find challenge
						auto challenge{ game::StringTable_GetColumnValueForRow(challenges_table, i, 0) };

						int max_state{ 0 };
						int max_progress{ 0 };

						// Find correct tier and progress
						for (int j = 0; j < 8; ++j)
						{
							int progress{ atoi(game::StringTable_GetColumnValueForRow(challenges_table, i, 9 + j * 2)) };
							if (!progress) break;

							max_state = j + 2;
							max_progress = progress;
						}

						nav_strings[0] = game::SL_FindString("challengeState");
						nav_strings[1] = game::SL_FindString(challenge);

						game::LiveStorage_PlayerDataSetIntByNameArray(
							controller_index, nav_strings, 2, max_state, game::STATSGROUP_RANKED);

						nav_strings[0] = game::SL_FindString("challengeProgress");
						game::LiveStorage_PlayerDataSetIntByNameArray(
							controller_index, nav_strings, 2, max_progress, game::STATSGROUP_RANKED);
					}
				}

				/**
				* UNLOCKING PAST TITLE DATA
				**/

				nav_strings[0] = game::SL_FindString("pastTitleData");
				nav_strings[1] = game::SL_FindString("playedmw3");

				game::LiveStorage_PlayerDataSetIntByNameArray(
					controller_index, nav_strings, 2, 1, game::STATSGROUP_COMMON);

				nav_strings[1] = game::SL_FindString("playedblackops2");

				game::LiveStorage_PlayerDataSetIntByNameArray(
					controller_index, nav_strings, 2, 1, game::STATSGROUP_COMMON);

				nav_strings[1] = game::SL_FindString("mw3prestige");

				game::LiveStorage_PlayerDataSetIntByNameArray(
					controller_index, nav_strings, 2, 20, game::STATSGROUP_COMMON);

				nav_strings[1] = game::SL_FindString("blackops2prestige");

				game::LiveStorage_PlayerDataSetIntByNameArray(
					controller_index, nav_strings, 2, 11, game::STATSGROUP_COMMON);

				//This is necessary for the stats to stick after closing the game
				auto* stat_write_needed = reinterpret_cast<bool*>(0x1445A7C72);
				if (stat_write_needed)
				{
					*stat_write_needed = true;
				}
			});
		}
	};
}

REGISTER_COMPONENT(stats::component)
