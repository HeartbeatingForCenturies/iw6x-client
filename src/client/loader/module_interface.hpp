#pragma once

class module_interface
{
public:
	virtual ~module_interface()
	{
	}

	virtual void post_start()
	{
	}

	virtual void post_load()
	{
	}

	virtual void pre_destroy()
	{
	}

	virtual void post_unpack()
	{
	}

	virtual void* load_import([[maybe_unused]] const std::string& module, [[maybe_unused]] const std::string& function)
	{
		return nullptr;
	}
};
