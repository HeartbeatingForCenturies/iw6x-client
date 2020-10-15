#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "scheduler.hpp"
#include "game/game.hpp"
#include "utils/concurrent_list.hpp"
#include "utils/hook.hpp"

namespace scheduler
{
	namespace
	{
		struct task
		{
			pipeline type;
			std::function<bool()> handler;
			std::chrono::milliseconds interval;
			std::chrono::high_resolution_clock::time_point last_call{};
		};

		volatile bool kill = false;
		std::thread thread;
		utils::concurrent_list<task> callbacks;
		utils::hook::detour r_end_frame_hook;

		void execute(const pipeline type)
		{
			for (auto callback : callbacks)
			{
				if (callback->type != type)
				{
					continue;
				}

				const auto now = std::chrono::high_resolution_clock::now();
				const auto diff = now - callback->last_call;

				if (diff < callback->interval) continue;

				callback->last_call = now;

				const auto res = callback->handler();
				if (res == cond_end)
				{
					callbacks.remove(callback);
				}
			}
		}

		void r_end_frame_stub()
		{
			execute(pipeline::renderer);
			r_end_frame_hook.invoke<void>();
		}

		int server_frame_stub(const int server_time)
		{
			execute(pipeline::server);
			return game::G_RunFrame(server_time);
		}

		void main_frame_stub()
		{
			execute(pipeline::main);
			game::Com_Frame_Try_Block_Function();
		}
	}

	void schedule(const std::function<bool()>& callback, const pipeline type,
	              const std::chrono::milliseconds delay)
	{
		task task;
		task.type = type;
		task.handler = callback;
		task.interval = delay;
		task.last_call = std::chrono::high_resolution_clock::now();

		callbacks.add(task);
	}

	void loop(const std::function<void()>& callback, const pipeline type,
	          const std::chrono::milliseconds delay)
	{
		schedule([callback]()
		{
			callback();
			return cond_continue;
		}, type, delay);
	}

	void once(const std::function<void()>& callback, const pipeline type,
	          const std::chrono::milliseconds delay)
	{
		schedule([callback]()
		{
			callback();
			return cond_end;
		}, type, delay);
	}

	class module final : public module_interface
	{
	public:
		void post_start() override
		{
			thread = std::thread([]()
			{
				while (!kill)
				{
					execute(pipeline::async);
					std::this_thread::sleep_for(10ms);
				}
			});
		}

		void post_unpack() override
		{
			r_end_frame_hook.create(SELECT_VALUE(0x140534860, 0x140601AA0), scheduler::r_end_frame_stub);

			utils::hook::call(SELECT_VALUE(0x1403BC922, 0x140413142), scheduler::main_frame_stub);

			// Server thread isn't really a thing in SP, at least I couldn't find what would be the equivalent
			if (!game::environment::is_sp())
			{
				utils::hook::call(0x14047A4C2, scheduler::server_frame_stub);
				utils::hook::call(0x14047B035, scheduler::server_frame_stub);
			}
		}

		void pre_destroy() override
		{
			kill = true;
			if (thread.joinable())
			{
				thread.join();
			}
		}
	};
}

REGISTER_MODULE(scheduler::module)
