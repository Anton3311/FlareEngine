local build_tool = require("BuildTool")

project "FlareECS"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	build_tool.define_module("FlareECS")
	build_tool.add_module_ref("FlareCore")

    files
    {
        "src/**.h",
        "src/**.cpp",
    }

    includedirs
	{
		"src/",
		"include/",
		"%{wks.location}/FlareCore/src/",
		INCLUDE_DIRS.spdlog,
		INCLUDE_DIRS.glm,
		INCLUDE_DIRS.tracy
	}

	links
	{
		"FlareCore",
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "FLARE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines { "FLARE_RELEASE", "TRACY_ENABLE", "TRACY_IMPORTS" }
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "FLARE_DIST"
		runtime "Release"
		optimize "on"

