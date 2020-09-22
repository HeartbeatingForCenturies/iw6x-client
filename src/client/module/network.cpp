#include <std_include.hpp>
#include "network.hpp"

#include "command.hpp"
#include "game/game.hpp"
#include "utils/hook.hpp"

namespace
{
	std::unordered_map<std::string, network::callback>& get_callbacks()
	{
		static std::unordered_map<std::string, network::callback> callbacks{};
		return callbacks;
	}

	bool handle_command(game::native::netadr_s* address, const char* command, game::native::msg_t* message)
	{
		auto& callbacks = get_callbacks();
		const auto handler = callbacks.find(command);
		if (handler == callbacks.end())
		{
			return false;
		}

		const auto offset = strlen(command) + 5;
		const std::string_view data(message->data + offset, message->cursize - offset);

		handler->second(*address, data);
		return true;
	}

	void handle_command_stub(utils::hook::assembler& a)
	{
		const auto return_unhandled = a.newLabel();

		a.pushad64();

		a.mov(r8, rsi); // message
		a.mov(rdx, rdi); // command
		a.xor_(rcx, r14); // netaddr

		a.call(handle_command);

		a.test(ax, ax);
		a.jz(return_unhandled);

		// Command handled
		a.popad64();
		a.jmp(0x1402C6DE8);

		a.bind(return_unhandled);
		a.popad64();
		a.jmp(0x1402C64EE);
	}

	enum class compare_result : std::int32_t
	{
		equal = 1,
		not_equal = 0,
	};

	compare_result net_compare_base_address(game::native::netadr_s* a1, game::native::netadr_s* a2)
	{
		if (a1->type == a2->type)
		{
			switch (a1->type)
			{
			case game::native::netadrtype_t::NA_BOT:
			case game::native::netadrtype_t::NA_LOOPBACK:
				return (a1->port == a2->port) ? compare_result::equal : compare_result::not_equal;

			case game::native::netadrtype_t::NA_IP:
				return (!memcmp(a1->ip, a2->ip, 4)) ? compare_result::equal : compare_result::not_equal;
			case game::native::netadrtype_t::NA_BROADCAST:
				return compare_result::equal;
			default:
				break;
			}
		}

		return compare_result::not_equal;
	}

	compare_result net_compare_address(game::native::netadr_s* a1, game::native::netadr_s* a2)
	{
		if (net_compare_base_address(a1, a2) == compare_result::equal)
		{
			if (a1->port == a2->port)
			{
				return compare_result::equal;
			}
		}

		// not equals
		return compare_result::not_equal;
	}
}

void network::on(const std::string& command, const callback& callback)
{
	get_callbacks()[command] = callback;
}

void network::send(const game::native::netadr_s& address, const std::string& command, const std::string& data)
{
	std::string packet = "\xFF\xFF\xFF\xFF";
	packet.append(command);
	packet.push_back(' ');
	packet.append(data);

	network::send(address, packet);
}

void network::send(const game::native::netadr_s& address, const std::string& data)
{
	game::native::Sys_SendPacket(static_cast<int>(data.size()), data.data(), &address);
}

void network::post_unpack()
{
	// redirect dw_sendto to raw socket
	utils::hook::jump(0x140501AAA, reinterpret_cast<void*>(0x140501A3A));

	// intercept command handling
	utils::hook::jump(0x1402C64CB, utils::hook::assemble(handle_command_stub), true);

	utils::hook::jump(0x14041D010, net_compare_address);
	utils::hook::jump(0x14041D060, net_compare_base_address);

	// don't establish secure conenction
	utils::hook::set<uint8_t>(0x1402ECF1D, 0xEB);
	utils::hook::set<uint8_t>(0x1402ED02A, 0xEB);
	utils::hook::set<uint8_t>(0x1402ED34D, 0xEB);
	utils::hook::set<uint8_t>(0x1402C4A1F, 0xEB); //

	// ignore unregistered connection
	utils::hook::jump(0x140471AAC, reinterpret_cast<void*>(0x140471A50));
	utils::hook::set<uint8_t>(0x140471AF0, 0xEB);

	// disable xuid verification
	utils::hook::set<uint8_t>(0x140154AA9, 0xEB);
	utils::hook::set<uint8_t>(0x140154AC3, 0xEB);

	command::add("lul", [](command::params&)
	{
		game::native::netadr_s addr{};
		addr.port = htons(27016);
		addr.type = game::native::NA_IP;
		inet_pton(AF_INET, "192.168.10.111", addr.ip);

		network::send(addr, "test_command", "this is a test");
	});

	command::add("hehe", [](command::params&)
	{
		char session_info[0x1000] = {};

		game::native::netadr_s addr{};
		addr.port = htons(27016);
		addr.type = game::native::NA_IP;
		inet_pton(AF_INET, "192.168.10.19", addr.ip);

		// CL_ConnectFromParty
		((void(*)(int, char*, game::native::netadr_s*, const char*, const char*))0x1402C5700)(
			0, session_info, &addr, "mp_prisonbreak", "war");
	});

	network::on("test_command", [](const game::native::netadr_s& address, const std::string_view& data)
	{
		printf("YEEEEEEEEEEES: %s\n", std::string(data).data());
	});
}

#if defined(DEV_BUILD)
REGISTER_MODULE(network)
#endif
