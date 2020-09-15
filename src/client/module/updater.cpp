#include <std_include.hpp>
#include "loader/module_loader.hpp"
#include "utils/io.hpp"
#include "utils/nt.hpp"

#include <version.h>

namespace
{
	bool is_update_available()
	{
		return false;
	}

	void perform_update()
	{
	}

	bool try_updating()
	{
		const utils::nt::module self;
		const auto self_file = self.get_path();
		const auto dead_file = self_file + ".old";

		utils::io::remove_file(dead_file);

		if (!is_update_available())
		{
			return false;
		}

		utils::io::move_file(self_file, dead_file);
		perform_update();

		return true;
	}
}

class updater final : public module
{
public:
	void post_start() override
	{
		if (try_updating())
		{
			module_loader::trigger_premature_shutdown();
		}
	}
};

#ifndef DEV_BUILD
REGISTER_MODULE(updater)
#endif
