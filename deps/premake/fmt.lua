fmt = {
	source = path.join(dependencies.basePath, "fmt"),
}

function fmt.import()
	links { "fmt" }

	fmt.includes()
end

function fmt.includes()
	includedirs {
		path.join(fmt.source, "include"),
	}
end

function fmt.project()
	project "fmt"
	kind "StaticLib"
	language "C++"

	fmt.includes()

	files {
		path.join(fmt.source, "include/fmt/*.h"),
		path.join(fmt.source, "src/*.cc")
	}

	removefiles {
		path.join(fmt.source, "src/fmt.cc")
	}
end

table.insert(dependencies, fmt)
