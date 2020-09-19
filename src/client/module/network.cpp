#include <std_include.hpp>
#include "network.hpp"

#include "command.hpp"
#include "scheduler.hpp"
#include "steam/steam.hpp"
#include "utils/string.hpp"

#define DISCOVERY_PORT 9000
#define DISCOVERY_RANGE 100

uint64_t network::id_;
SOCKET network::socket_;

void network::address::set_ipv4(const ULONG addr)
{
	this->address_.sin_family = AF_INET;
	this->address_.sin_addr.s_addr = addr;
}

void network::address::set_port(const unsigned short port)
{
	this->address_.sin_port = htons(port);
}

unsigned short network::address::get_port() const
{
	return ntohs(this->address_.sin_port);
}

void network::address::parse(std::string addr)
{
	const auto pos = addr.find_first_of(':');
	if (pos != std::string::npos)
	{
		const auto port = addr.substr(pos + 1);
		this->set_port(USHORT(atoi(port.data())));

		addr = addr.substr(0, pos);
	}

	this->resolve(addr);
}

void network::address::resolve(const std::string& hostname)
{
	addrinfo* result = nullptr;
	if (!getaddrinfo(hostname.data(), nullptr, nullptr, &result))
	{
		const auto port = this->get_port();
		std::memcpy(&this->address_, result->ai_addr, size_t(this->size()));
		this->set_port(port);

		freeaddrinfo(result);
	}
}

std::map<std::string, network::network_callback>& network::get_callbacks()
{
	static std::map<std::string, network::network_callback> callbacks;
	return callbacks;
}

void network::run_frame()
{
	char buffer[0x2000];

	sockaddr_in addr;
	int addr_len = sizeof(addr);

	while (true)
	{
		const auto len = recvfrom(network::socket_, buffer, sizeof buffer, 0, reinterpret_cast<sockaddr*>(&addr),
		                          &addr_len);
		if (len <= 0) break;

		network::address address(&addr);

		proto::network::unique_packet packet;
		if (packet.ParseFromArray(buffer, len) && packet.id() != network::id_)
		{
			auto& callbacks = network::get_callbacks();
			if (callbacks.find(packet.packet().command()) != callbacks.end())
			{
				callbacks[packet.packet().command()](address, packet.packet().payload());
			}
		}
	}
}

void network::send(const network::address& address, const std::string& command, const std::string& data)
{
	proto::network::unique_packet packet;
	packet.mutable_packet()->set_command(command);
	packet.mutable_packet()->set_payload(data);
	packet.set_id(network::id_);

	const auto buffer = packet.SerializeAsString();
	sendto(network::socket_, buffer.data(), INT(buffer.size()), 0, address.get_sock_addr(), address.size());
}

SOCKET network::get_socket()
{
	return network::socket_;
}

void network::broadcast(const std::string& command, const std::string& data)
{
	auto addresses = network::get_broadcast_addresses();

	for (auto& address : addresses)
	{
		for (unsigned short i = 0; i < DISCOVERY_RANGE; ++i)
		{
			address.set_port(i + DISCOVERY_PORT);
			network::send(address, command, data);
		}
	}
}

void network::on(const std::string& command, const network_callback& handler)
{
	network::get_callbacks()[command] = handler;
}

bool network::calculate_broadcast_address(IP_ADAPTER_INFO& adapter, network::address& address)
{
	network::address ip(adapter.IpAddressList.IpAddress.String);
	network::address mask(adapter.IpAddressList.IpMask.String);

	if (ip.get()->sin_family != AF_INET || mask.get()->sin_family != AF_INET)
	{
		return false;
	}

	const auto ip_value = ip.get()->sin_addr.S_un.S_addr;
	const auto mask_value = mask.get()->sin_addr.S_un.S_addr;
	const auto network_value = ip_value & mask_value;
	const auto broadcast_value = network_value | ~mask_value;

	address.set_ipv4(broadcast_value);

	return true;
}

std::vector<IP_ADAPTER_INFO> network::get_adapter_infos()
{
	std::vector<IP_ADAPTER_INFO> result;

	utils::memory::allocator allocator;
	PIP_ADAPTER_INFO adapter_infos = allocator.allocate<IP_ADAPTER_INFO>();
	DWORD size = sizeof(IP_ADAPTER_INFO);

	if (GetAdaptersInfo(adapter_infos, &size) == ERROR_BUFFER_OVERFLOW)
	{
		allocator.free(adapter_infos);
		adapter_infos = PIP_ADAPTER_INFO(allocator.allocate_array<char>(size));
	}

	if (GetAdaptersInfo(adapter_infos, &size) == NO_ERROR)
	{
		while (adapter_infos)
		{
			result.push_back(*adapter_infos);
			adapter_infos = adapter_infos->Next;
		}
	}

	return result;
}

std::vector<network::address> network::get_broadcast_addresses()
{
	std::vector<network::address> result;

	auto adapter_infos = get_adapter_infos();

	for (auto& adapter_info : adapter_infos)
	{
		network::address broadcast;
		if (calculate_broadcast_address(adapter_info, broadcast))
		{
			result.push_back(broadcast);
		}
	}

	return result;
}

network::~network()
{
	closesocket(network::socket_);
	WSACleanup();
}

void network::post_start()
{
	network::id_ = steam::SteamUser()->GetSteamID().bits;

	WSAData data{};
	WSAStartup(MAKEWORD(2, 2), &data);

	network::socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	int broadcast_on = 1;
	setsockopt(network::socket_, SOL_SOCKET, SO_BROADCAST, LPSTR(&broadcast_on), sizeof(broadcast_on));

	unsigned long non_blocking = 1;
	ioctlsocket(network::socket_, FIONBIO, &non_blocking);

	sockaddr_in addr{};
	ZeroMemory(&addr, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;

	unsigned short port = DISCOVERY_PORT;
	while (port < DISCOVERY_PORT + DISCOVERY_RANGE)
	{
		addr.sin_port = htons(port++);
		if (bind(network::socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != SOCKET_ERROR)
		{
			printf("Socket bound to port %hu\n", ntohs(addr.sin_port));
			break;
		}
	}

	scheduler::loop(network::run_frame);

	scheduler::once([]()
	{
		command::add("echo", [](command::params& params)
		{
			if (params.size() == 2)
			{
				printf("Sending echo ping to target...\n");
				const network::address address(params[1]);
				network::send(address, "echo", "test");
			}
			else
			{
				printf("Sending echo ping...\n");
				network::broadcast("echo", "test");
			}
		});
	}, scheduler::pipeline::renderer);

	network::on("echo", [](const network::address& address, const std::string& data)
	{
		printf("Received echo ping. Sending reply...\n");
		network::send(address, "echo_reply", data);
	});

	network::on("echo_reply", [](const network::address& address, const std::string& data)
	{
		printf("Received echo reply!\n");
	});
}

REGISTER_MODULE(network)
