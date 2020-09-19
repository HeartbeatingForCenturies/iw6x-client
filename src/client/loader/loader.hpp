#pragma once
#include "utils/nt.hpp"
#include "launcher/launcher.hpp"

class loader final
{
public:
	FARPROC load(const utils::nt::module& module, const std::string& buffer) const;

	void set_import_resolver(const std::function<void*(const std::string&, const std::string&)>& resolver);

private:
	std::function<void*(const std::string&, const std::string&)> import_resolver_;

	static void load_section(const utils::nt::module& target, const utils::nt::module& source,
	                         IMAGE_SECTION_HEADER* section);
	void load_sections(const utils::nt::module& target, const utils::nt::module& source) const;
	void load_imports(const utils::nt::module& target, const utils::nt::module& source) const;
	void load_exception_table(const utils::nt::module& target, const utils::nt::module& source) const;
};
