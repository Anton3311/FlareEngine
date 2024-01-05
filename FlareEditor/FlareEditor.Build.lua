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

		INCLUDE_DIRS.vulkan_sdk,
		INCLUDE_DIRS.tracy,
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

			LIBRARIES.shaderc_debug,
			LIBRARIES.spriv_cross_glsl_debug,
			LIBRARIES.spriv_cross_debug,
			LIBRARIES.spriv_tools_debug,
		}

	filter "configurations:Release"
		defines { "FLARE_RELEASE", "TRACY_ENABLE", "TRACY_IMPORTS" }
		runtime "Release"
		optimize "on"

		links
		{
			LIBRARIES.assimp_release,
			LIBRARIES.assimp_zlib_release,

			LIBRARIES.shaderc_release,
			LIBRARIES.spriv_cross_glsl_release,
			LIBRARIES.spriv_cross_release,


			LIBRARIES.assimp_debug,
			LIBRARIES.assimp_zlib_debug,
		}

	filter "configurations:Dist"
		defines "FLARE_DIST"
		runtime "Release"
		optimize "on"

		links
		{
			LIBRARIES.assimp_release,
			LIBRARIES.assimp_zlib_release,

			LIBRARIES.shaderc_release,
			LIBRARIES.spriv_cross_glsl_release,
			LIBRARIES.spriv_cross_release,

			LIBRARIES.assimp_release,
			LIBRARIES.assimp_zlib_release,
		}
