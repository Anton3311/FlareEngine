local build_tool = require("BuildTool")

project "FlareCore"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	build_tool.define_module("FlareCore")

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
		INCLUDE_DIRS.tracy,
	}

	filter "configurations:Release"
		files { "%{wks.location}/Flare/vendor/Tracy/TracyClient.cpp" }
		includedirs
		{
			"%{wks.location}/Flare/vendor/Tracy/",
		}

		defines { "TRACY_ENABLE", "TRACY_EXPORTS" }

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
