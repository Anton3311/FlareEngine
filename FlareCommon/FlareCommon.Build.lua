local build_tool = require("BuildTool")

project "FlareCommon"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	build_tool.define_module("FlareCommon")

    files
    {
        "src/**.h",
        "src/**.cpp",
    }

    includedirs
	{
		"src/",
		INCLUDE_DIRS.spdlog,
		INCLUDE_DIRS.glm,
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
