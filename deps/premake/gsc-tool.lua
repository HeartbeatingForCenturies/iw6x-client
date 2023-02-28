gsc_tool = {
	source = path.join(dependencies.basePath, "gsc-tool/src"),
}

function gsc_tool.import()
	links { "xsk-gsc-iw6", "xsk-gsc-utils" }
	gsc_tool.includes()
end

function gsc_tool.includes()
	includedirs {
		gsc_tool.source,
	}
end

function gsc_tool.project()
	project "xsk-gsc-utils"
		kind "StaticLib"
		language "C++"

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
			buildoptions "/Zc:__cplusplus"
		filter {}

		files {
			path.join(gsc_tool.source, "stdinc.hpp"),

			path.join(gsc_tool.source, "iw6/iw6_pc.hpp"),
			path.join(gsc_tool.source, "iw6/iw6_pc.cpp"),
			path.join(gsc_tool.source, "iw6/iw6_pc_code.cpp"),
			path.join(gsc_tool.source, "iw6/iw6_pc_func.cpp"),
			path.join(gsc_tool.source, "iw6/iw6_pc_meth.cpp"),
			path.join(gsc_tool.source, "iw6/iw6_pc_token.cpp"),

			path.join(gsc_tool.source, "gsc/misc/*.hpp"),
			path.join(gsc_tool.source, "gsc/misc/*.cpp"),
			path.join(gsc_tool.source, "gsc/*.hpp"),
			path.join(gsc_tool.source, "gsc/*.cpp"),
		}

		includedirs {
			gsc_tool.source,
		}
end

table.insert(dependencies, gsc_tool)
