local build_tool = require("BuildTool")

project "FlarePlatform"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	build_tool.define_module("FlarePlatform")
	build_tool.add_module_ref("FlareCommon")

    files
    {
        "src/**.h",
        "src/**.cpp",
    }

    includedirs
	{
		"src/",
		"%{wks.location}/FlareCommon/src/",

		INCLUDE_DIRS.GLFW,
		INCLUDE_DIRS.glm,
		INCLUDE_DIRS.spdlog,
	}

	links
	{
		"GLFW",
		"ImGUI",
		"FlareCommon",
	}

	defines
	{
		"GLFW_INCLUDE_NONE",
		"_GLFW_BUILD_DLL",
	}

	filter "system:windows"
		systemversion "latest"

		links { "dwmapi.lib" }
	
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
