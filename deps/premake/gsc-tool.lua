gsc_tool = {
	source = path.join(dependencies.basePath, "gsc-tool/src"),
}

function gsc_tool.import()
	links { "xsk-gsc-iw6", "xsk-gsc-utils" }
	gsc_tool.includes()
end

function gsc_tool.includes()
	includedirs {
		path.join(gsc_tool.source, "iw6"),
		path.join(gsc_tool.source, "utils"),
		path.join(dependencies.basePath, "extra/gsc-tool"),
	}
end

function gsc_tool.project()
	project "xsk-gsc-utils"
		kind "StaticLib"
		language "C++"

		pchheader "stdafx.hpp"
		pchsource (path.join(gsc_tool.source, "utils/stdafx.cpp"))

		files {
			path.join(gsc_tool.source, "utils/**.hpp"),
			path.join(gsc_tool.source, "utils/**.cpp"),
		}

		includedirs {
			path.join(gsc_tool.source, "utils"),
			gsc_tool.source,
		}

		zlib.includes()

	project "xsk-gsc-iw6"
		kind "StaticLib"
		language "C++"

		filter "action:vs*"
			buildoptions "/bigobj"
			buildoptions "/Zc:__cplusplus"
		filter {}

		pchheader "stdafx.hpp"
		pchsource (path.join(gsc_tool.source, "iw6/stdafx.cpp"))

		files {
			path.join(gsc_tool.source, "iw6/**.hpp"),
			path.join(gsc_tool.source, "iw6/**.cpp"),
			path.join(dependencies.basePath, "extra/gsc-tool/interface.cpp"),
		}

		includedirs {
			path.join(gsc_tool.source, "iw6"),
			gsc_tool.source,
			path.join(dependencies.basePath, "extra/gsc-tool"),
		}
end

table.insert(dependencies, gsc_tool)
