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

	command::add("lul", [](command::params&)
	{
		game::native::netadr_s addr{};
		addr.port = htons(27016);
		addr.type = game::native::NA_IP;
		inet_pton(AF_INET, "192.168.10.111", addr.ip);

		network::send(addr, "test_command", "this is a test");
	});

	network::on("test_command", [](const game::native::netadr_s& address, const std::string_view& data)
	{
		printf("YEEEEEEEEEEES: %s\n", std::string(data).data());
	});
}

REGISTER_MODULE(network)
