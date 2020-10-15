#include <std_include.hpp>
#include "module_loader.hpp"

void module_loader::register_module(std::unique_ptr<module_interface>&& module_)
{
	get_modules().push_back(std::move(module_));
}

bool module_loader::post_start()
{
	static auto handled = false;
	if (handled) return true;
	handled = true;

	try
	{
		for (const auto& module_ : get_modules())
		{
			module_->post_start();
		}
	}
	catch (premature_shutdown_trigger&)
	{
		return false;
	}

	return true;
}

bool module_loader::post_load()
{
	static auto handled = false;
	if (handled) return true;
	handled = true;

	try
	{
		for (const auto& module_ : get_modules())
		{
			module_->post_load();
		}
	}
	catch (premature_shutdown_trigger&)
	{
		return false;
	}

	return true;
}

void module_loader::post_unpack()
{
	static auto handled = false;
	if (handled) return;
	handled = true;

	for (const auto& module_ : get_modules())
	{
		module_->post_unpack();
	}
}

void module_loader::pre_destroy()
{
	static auto handled = false;
	if (handled) return;
	handled = true;

	for (const auto& module_ : get_modules())
	{
		module_->pre_destroy();
	}
}

void* module_loader::load_import(const std::string& module, const std::string& function)
{
	void* function_ptr = nullptr;

	for (const auto& module_ : get_modules())
	{
		const auto module_function_ptr = module_->load_import(module, function);
		if (module_function_ptr)
		{
			function_ptr = module_function_ptr;
		}
	}

	return function_ptr;
}

void module_loader::trigger_premature_shutdown()
{
	throw premature_shutdown_trigger();
}

std::vector<std::unique_ptr<module_interface>>& module_loader::get_modules()
{
	using module_vector = std::vector<std::unique_ptr<module_interface>>;
	using module_vector_container = std::unique_ptr<module_vector, std::function<void(module_vector*)>>;

	static
		module_vector_container modules(new module_vector, [](module_vector* module_vector)
		{
			pre_destroy();
			delete module_vector;
		});

	return *modules;
}
