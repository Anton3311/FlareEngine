local build_tool = require("BuildTool")

project "Flare"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	build_tool.define_module("Flare")
	build_tool.add_module_ref("FlarePlatform")
	build_tool.add_module_ref("FlareCommon")
	build_tool.add_module_ref("FlareECS")

    files
    {
        "src/**.h",
        "src/**.cpp",

		"vendor/stb_image/stb_image/**.h",
		"vendor/stb_image/stb_image/**.cpp",
    }

    includedirs
	{
		"src/",
		"%{wks.location}/FlareCommon/src/",
		"%{wks.location}/FlarePlatform/src/",
		"%{wks.location}/FlareECS/src/",
		"%{wks.location}/FlareECS/include/",
		"%{wks.location}/FlareScriptingCore/src/",

		INCLUDE_DIRS.GLAD,
		INCLUDE_DIRS.GLFW,
		INCLUDE_DIRS.glm,
		INCLUDE_DIRS.stb_image,
		INCLUDE_DIRS.spdlog,
		INCLUDE_DIRS.imguizmo,
		INCLUDE_DIRS.yaml_cpp,
		INCLUDE_DIRS.imgui,
	}

	links
	{
		"GLAD",
		"GLFW",
		"ImGUI",
		"FlareECS",
		"FlareCommon",
		"FlarePlatform",
		"FlareScriptingCore",
		"yaml-cpp"
	}

	defines
	{
		"GLFW_INCLUDE_NONE",
		"YAML_CPP_STATIC_DEFINE",
		"FLARE_SCRIPTING_CORE_NO_MACROS",
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
