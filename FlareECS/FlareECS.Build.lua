local build_tool = require("BuildTool")

project "FlareECS"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	build_tool.define_module("FlareECS")
	build_tool.add_module_ref("FlareCommon")

    files
    {
        "src/**.h",
        "src/**.cpp",

		"include/**.h",
    }

    includedirs
	{
		"src/",
		"include/",
		"%{wks.location}/FlareCommon/src/",
		INCLUDE_DIRS.spdlog,
		INCLUDE_DIRS.glm,
	}

	links
	{
		"FlareCommon",
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "FLARE_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "FLARE_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "FLARE_DIST"
		runtime "Release"
		optimize "on"

