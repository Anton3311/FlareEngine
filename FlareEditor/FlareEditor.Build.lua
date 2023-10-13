local build_tool = require("BuildTool")

project "FlareEditor"
    kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	build_tool.add_module_ref("Flare")
	build_tool.add_module_ref("FlarePlatform")
	build_tool.add_module_ref("FlareCore")
	build_tool.add_module_ref("FlareECS")

    files
    {
        "src/**.h",
        "src/**.cpp",

		"%{wks.location}/Flare/vendor/ImGuizmo/ImGuizmo.h",
		"%{wks.location}/Flare/vendor/ImGuizmo/ImGuizmo.cpp",
    }

    includedirs
    {
        "src",
		"%{wks.location}/Flare/src",
		"%{wks.location}/FlareCore/src",
		"%{wks.location}/FlarePlatform/src",
		"%{wks.location}/FlareECS/src",

		INCLUDE_DIRS.msdf_gen,
		INCLUDE_DIRS.msdf_atlas_gen,

		INCLUDE_DIRS.assimp,

		INCLUDE_DIRS.glm,
		INCLUDE_DIRS.GLFW,
		INCLUDE_DIRS.spdlog,
		INCLUDE_DIRS.imgui,
		INCLUDE_DIRS.imguizmo,
		INCLUDE_DIRS.yaml_cpp,
    }

	links
	{
		"Flare",
		"GLFW",
		"ImGUI",
		"FlareCore",
		"FlarePlatform",
		"FlareECS",
		"yaml-cpp",
	}

	defines
	{
		"YAML_CPP_STATIC_DEFINE",
	}

	debugargs
	{
		"%{wks.location}/Sandbox/Sandbox.flareproj"
	}

	targetdir("%{wks.location}/bin/" .. OUTPUT_DIRECTORY)
	objdir("%{wks.location}/bin-int/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "FLARE_DEBUG"
		runtime "Debug"
		symbols "on"

		links
		{
			LIBRARIES.assimp_debug,
			LIBRARIES.assimp_zlib_debug,
		}

	filter "configurations:Release"
		defines "FLARE_RELEASE"
		runtime "Release"
		optimize "on"

		links
		{
			LIBRARIES.assimp_release,
			LIBRARIES.assimp_zlib_release,
		}

	filter "configurations:Dist"
		defines "FLARE_DIST"
		runtime "Release"
		optimize "on"

		links
		{
			LIBRARIES.assimp_release,
			LIBRARIES.assimp_zlib_release,
		}
