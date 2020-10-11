#pragma once
#include "loader/module_loader.hpp"
#include "utils/concurrent_list.hpp"
#include "utils/hook.hpp"

class scheduler final : public module
{	
public:
	enum class pipeline
	{
		// Asynchronuous pipeline, disconnected from the game
		async,

		// The game's rendering pipeline
		renderer,

		// The game's server thread
		server,

		// The game's main thread
		main,
	};
	
	static const bool cond_continue = false;
	static const bool cond_end = true;
	
	static void schedule(const std::function<bool()>& callback, pipeline type = pipeline::async, std::chrono::milliseconds delay = 0ms);
	static void loop(const std::function<void()>& callback, pipeline type = pipeline::async, std::chrono::milliseconds delay = 0ms);
	static void once(const std::function<void()>& callback, pipeline type = pipeline::async, std::chrono::milliseconds delay = 0ms);

	void post_start() override;
	void post_unpack() override;
	void pre_destroy() override;

private:
	struct task
	{
		pipeline type;
		std::function<bool()> handler;
		std::chrono::milliseconds interval;
		std::chrono::high_resolution_clock::time_point last_call{};
	};
	
	volatile bool kill_ = false;
	std::thread thread_;
	
	utils::concurrent_list<task> callbacks_;

	void execute(pipeline pipeline);

	static void r_end_frame_stub();
	static int server_frame_stub(const int server_time);
	static void main_frame_stub();
};
