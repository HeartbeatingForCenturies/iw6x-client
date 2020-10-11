#include <std_include.hpp>

#include "command.hpp"
#include "scheduler.hpp"
#include "game/game.hpp"

#include "utils/string.hpp"

namespace
{
	void spawn_bot(const int entity_num)
	{
		scheduler::once([entity_num]()
		{
			game::SV_SpawnTestClient(&game::mp::g_entities[entity_num]);

			// schedule the select team call
			scheduler::once([entity_num]()
			{
				game::SV_ExecuteClientCommand(&game::mp::svs_clients[entity_num], utils::string::va("lui 68 2 %i", *game::mp::sv_serverId_value), false);

				// scheduler the select class call
				scheduler::once([entity_num]()
				{
					game::SV_ExecuteClientCommand(&game::mp::svs_clients[entity_num], utils::string::va("lui 5 %i %i", (rand() % 5) + 10, *game::mp::sv_serverId_value), false);
				}, scheduler::pipeline::server, 1s);

			}, scheduler::pipeline::server, 1s);

		}, scheduler::pipeline::server, 1s);
	}
}

class bots final : public module
{
public:
	void post_unpack() override
	{
		if (game::environment::is_sp())
		{
			return;
		}

		// Disable bots kicked due to team balance
		utils::hook::nop(0x14046F70A, 5);

		command::add("spawnBot", [](command::params& params)
		{
			if (!game::SV_Loaded()) return;

			if (params.size() == 2)
			{
				auto num_bots = atoi(params.get(1));

				for (auto i = 0; i < num_bots; i++)
				{
					// SV_BotGetRandomName
					const char* bot_name = reinterpret_cast<const char*(*)()>(0x140460B80)();

					game::mp::gentity_s* bot_ent = game::SV_AddBot(bot_name, 26, 62, 0);

					if (bot_ent)
					{
						spawn_bot(bot_ent->s.entityNum);
					}
				}
			}
			else
			{
				// SV_BotGetRandomName
				const char* bot_name = reinterpret_cast<const char* (*)()>(0x140460B80)();

				game::mp::gentity_s* bot_ent = game::SV_AddBot(bot_name, 26, 62, 0);

				if (bot_ent)
				{
					spawn_bot(bot_ent->s.entityNum);
				}
			}
		});
	}
};

REGISTER_MODULE(bots);
