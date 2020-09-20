#include <std_include.hpp>
#include "lobby.hpp"
#include "network.hpp"

std::map<uint64_t, std::map<std::string, std::string>> lobby::data_;

void lobby::delete_lobby(const std::string& data)
{
	uint64_t id;
	if (data.size() >= sizeof(id))
	{
		id = *reinterpret_cast<const uint64_t*>(data.data());

		const auto lobby = lobby::data_.find(id);
		if (lobby != lobby::data_.end())
		{
			lobby::data_.erase(lobby);
		}
	}
}

void lobby::parse_lobby(const std::string& data)
{
	proto::network::unique_packet info;
	if (info.ParseFromString(data))
	{
		lobby::set_remote(info.id(), info.packet().command(), info.packet().payload());
	}
}

void lobby::remove(uint64_t lobby)
{
	std::string buffer;
	buffer.append(reinterpret_cast<char*>(&lobby), sizeof(lobby));

	network::broadcast("lobbyDelete", buffer);
}

void lobby::set_data(const uint64_t lobby, const std::string& key, const std::string& value)
{
	proto::network::unique_packet info;
	info.set_id(lobby);
	info.mutable_packet()->set_command(key);
	info.mutable_packet()->set_payload(value);

	network::broadcast("lobbyData", info.SerializeAsString());
}

std::string lobby::get_data(const uint64_t lobby, const std::string& key)
{
	auto lobby_data = lobby::data_.find(lobby);
	if (lobby_data != lobby::data_.end())
	{
		if (lobby_data->second.find(key) != lobby_data->second.end())
		{
			return lobby_data->second[key];
		}
	}

	return "data";
}

void lobby::set_remote(const uint64_t lobby, const std::string& key, const std::string& value)
{
	lobby::data_[lobby][key] = value;
	printf("Received data %s for lobby %lld\n", key.data(), lobby);
}

lobby::lobby()
{
	network::on("lobbyData", [](network::address, const std::string& data) { lobby::parse_lobby(data); });
	network::on("lobbyDelete", [](network::address, const std::string& data) { lobby::delete_lobby(data); });
}

lobby::~lobby()
{

}

REGISTER_MODULE(lobby)
