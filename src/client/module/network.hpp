#pragma once
#include "loader/module_loader.hpp"

class network final : public module
{
public:
	~network();

	void post_start() override;

	class address
	{
	public:
		address()
		{
			ZeroMemory(&this->address_, sizeof this->address_);
		}

		address(sockaddr* addr) : address() { std::memcpy(&this->address_, addr, sizeof this->address_); }

		address(sockaddr addr) : address(&addr)
		{
		}

		address(sockaddr_in* addr) : address(reinterpret_cast<sockaddr*>(addr))
		{
		}

		address(sockaddr_in addr) : address(reinterpret_cast<sockaddr*>(&addr))
		{
		}

		address(const std::string& addr) : address() { this->parse(addr); }

		void set_ipv4(ULONG addr);
		void set_port(unsigned short port);
		unsigned short get_port() const;

		sockaddr_in* get() { return &this->address_; }
		const sockaddr_in* get() const { return &this->address_; }

		sockaddr* get_sock_addr() { return reinterpret_cast<sockaddr*>(&this->address_); }
		const sockaddr* get_sock_addr() const { return reinterpret_cast<const sockaddr*>(&this->address_); }

		int size() const { return sizeof(this->address_); }

	private:
		void parse(std::string addr);
		void resolve(const std::string& hostname);

		sockaddr_in address_{};
	};

	using network_callback = std::function<void(const network::address&, const std::string&)>;

	static SOCKET get_socket();

	static void broadcast(const std::string& command, const std::string& data = "");
	static void send(const network::address& address, const std::string& command, const std::string& data = "");
	static void on(const std::string& command, const network_callback& handler);

	static bool calculate_broadcast_address(IP_ADAPTER_INFO& adapter, network::address& address);
	static std::vector<IP_ADAPTER_INFO> get_adapter_infos();
	static std::vector<network::address> get_broadcast_addresses();

private:
	static uint64_t id_;
	static SOCKET socket_;

	static std::map<std::string, network_callback>& get_callbacks();
	static void run_frame();
};
