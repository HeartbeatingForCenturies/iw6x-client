#include <std_include.hpp>
#include "bdMatchMaking.hpp"
#include "../data_types.hpp"
#include "Steam/Steam.hpp"

namespace demonware
{
	std::map<uint64_t, std::shared_ptr<MatchMakingInfo>> sessions;

	void UpdateSession(const std::string& data)
	{
		byte_buffer buffer(data);

		auto mmInfo = std::make_shared<MatchMakingInfo>();
		mmInfo->symmetric = true;
		mmInfo->deserialize(&buffer);
		mmInfo->symmetric = false;

		sessions[mmInfo->session_id.session_id] = mmInfo;
	}

	void DeleteSession(const std::string& data)
	{
		byte_buffer buffer(data);

		bdSessionID id;
		id.deserialize(&buffer);

		const auto session = sessions.find(id.session_id);
		if (session != sessions.end())
		{
			sessions.erase(session);
		}
	}

	bdMatchMaking::bdMatchMaking()
	{
		this->register_service(1, &bdMatchMaking::create_session);
		this->register_service(2, &bdMatchMaking::update_session);
		this->register_service(3, &bdMatchMaking::delete_session);
		this->register_service(10, &bdMatchMaking::get_performance);
		this->register_service(16, &bdMatchMaking::find_sessions_two_pass);

		//Components::Network::On("sessionUpdate", [](Components::Network::Address, std::string data) { UpdateSession(data); });
		//Components::IRCBridge::On("sessionUpdate", [](std::string, std::string data) { UpdateSession(data); });

		//Components::Network::On("sessionDelete", [](Components::Network::Address, std::string data) { DeleteSession(data); });
		//Components::IRCBridge::On("sessionDelete", [](std::string, std::string data) { DeleteSession(data); });
	}

	void bdMatchMaking::create_session(i_server* server, byte_buffer* /*buffer*/) const
	{
		auto* id = new bdSessionID;
		id->session_id = steam::SteamUser()->GetSteamID().bits;

		auto reply = server->create_reply(this->get_sub_type());
		reply->add(id);
		reply->send();
	}

	void bdMatchMaking::update_session(i_server* server, byte_buffer* buffer) const
	{
		MatchMakingInfo mmInfo;
		mmInfo.session_id.deserialize(buffer);
		mmInfo.deserialize(buffer);

		byte_buffer out_data;
		mmInfo.symmetric = true;
		mmInfo.serialize(&out_data);

		byte_buffer addr_buf(mmInfo.host_addr);
		bdCommonAddr addr;
		addr.deserialize(&addr_buf);

		//Components::DHT::SetPort(addr.m_publicAddr.m_port);

		//Components::Network::Broadcast("sessionUpdate", outData.getBuffer());
		//Components::IRCBridge::Broadcast("sessionUpdate", outData.getBuffer());

		auto reply = server->create_reply(this->get_sub_type());
		reply->send();
	}

	void bdMatchMaking::delete_session(i_server* server, byte_buffer* buffer) const
	{
		bdSessionID id;
		id.deserialize(buffer);

		byte_buffer out_data;
		id.serialize(&out_data);

		//Components::Network::Broadcast("sessionDelete", outData.getBuffer());

		auto reply = server->create_reply(this->get_sub_type());
		reply->send();
	}

	void bdMatchMaking::get_performance(i_server* server, byte_buffer* /*buffer*/) const
	{
		bdPerformanceValue* result = new bdPerformanceValue;
		result->user_id = steam::SteamUser()->GetSteamID().bits;
		result->performance = 10;

		auto reply = server->create_reply(this->get_sub_type());
		reply->add(result);
		reply->send();
	}

	void bdMatchMaking::find_sessions_two_pass(i_server* server, byte_buffer* /*buffer*/) const
	{
		auto reply = server->create_reply(this->get_sub_type());

		for (auto& session : sessions)
		{
			reply->add(session.second);
		}

		reply->send();
	}
}
