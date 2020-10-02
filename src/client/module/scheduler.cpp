#include <std_include.hpp>
#include "scheduler.hpp"
#include "game/game.hpp"

namespace
{
	utils::hook::detour r_end_frame_hook;
}

void scheduler::r_end_frame_stub()
{
	module_loader::get<scheduler>()->execute(pipeline::renderer);
	r_end_frame_hook.invoke<void>();
}

int scheduler::server_frame_stub(const int server_time)
{
	module_loader::get<scheduler>()->execute(pipeline::server);
	return game::G_RunFrame(server_time);
}

void scheduler::main_frame_stub()
{
	module_loader::get<scheduler>()->execute(pipeline::main);
	game::Com_Frame_Try_Block_Function();
}

void scheduler::schedule(const std::function<bool()>& callback, const pipeline type)
{
	auto* instance = module_loader::get<scheduler>();
	if (instance)
	{
		task task;
		task.type = type;
		task.handler = callback;

		instance->callbacks_.add(task);
	}
}

void scheduler::loop(const std::function<void()>& callback, const pipeline type)
{
	scheduler::schedule([callback]()
	{
		callback();
		return cond_continue;
	}, type);
}

void scheduler::once(const std::function<void()>& callback, const pipeline type)
{
	scheduler::schedule([callback]()
	{
		callback();
		return cond_end;
	}, type);
}

void scheduler::execute(const pipeline type)
{
	for (auto callback : this->callbacks_)
	{
		if (callback->type != type)
		{
			continue;
		}

		const auto res = callback->handler();
		if (res == cond_end)
		{
			this->callbacks_.remove(callback);
		}
	}
}

void scheduler::post_start()
{
	this->thread_ = std::thread([this]()
	{
		while (!this->kill_)
		{
			this->execute(pipeline::async);
			std::this_thread::sleep_for(10ms);
		}
	});
}

void scheduler::post_unpack()
{
	r_end_frame_hook.create(SELECT_VALUE(0x140534860, 0x140601AA0), scheduler::r_end_frame_stub);

	utils::hook::call(SELECT_VALUE(0x1403BC922, 0x140413142), scheduler::main_frame_stub);

	// Server thread isn't really a thing in SP, at least I couldn't find what would be the equivalent
	if (game::environment::is_mp())
	{
		utils::hook::call(0x14047A4C2, scheduler::server_frame_stub);
		utils::hook::call(0x14047B035, scheduler::server_frame_stub);
	}
}

void scheduler::pre_destroy()
{
	this->kill_ = true;
	if (this->thread_.joinable())
	{
		this->thread_.join();
	}
}

REGISTER_MODULE(scheduler);
