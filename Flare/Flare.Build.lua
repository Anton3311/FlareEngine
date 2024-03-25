local build_tool = require("BuildTool")

project "Flare"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

	build_tool.define_module("Flare")
	build_tool.add_module_ref("FlarePlatform")
	build_tool.add_module_ref("FlareCore")
	build_tool.add_module_ref("FlareECS")

    files
    {
        "src/**.h",
        "src/**.cpp",

		"vendor/stb_image/stb_image/**.h",
		"vendor/stb_image/stb_image/**.cpp",

		"vendor/VMA/vk_mem_alloc.h",
		"vendor/VMA/vk_mem_alloc.cpp",
    }

    includedirs
	{
		"src/",
		"%{wks.location}/FlareCore/src/",
		"%{wks.location}/FlarePlatform/src/",
		"%{wks.location}/FlareECS/src/",

		INCLUDE_DIRS.msdf_gen,
		INCLUDE_DIRS.msdf_atlas_gen,

		INCLUDE_DIRS.GLAD,
		INCLUDE_DIRS.GLFW,
		INCLUDE_DIRS.glm,
		INCLUDE_DIRS.stb_image,
		INCLUDE_DIRS.spdlog,
		INCLUDE_DIRS.imguizmo,
		INCLUDE_DIRS.yaml_cpp,
		INCLUDE_DIRS.imgui,

		INCLUDE_DIRS.vulkan_sdk,
		INCLUDE_DIRS.vma,
		INCLUDE_DIRS.assimp,
		INCLUDE_DIRS.tracy,
	}

	links
	{
		"GLAD",
		"GLFW",
		"ImGUI",
		"FlareECS",
		"FlareCore",
		"FlarePlatform",
		"yaml-cpp",

		"msdfgen",
		"msdf-atlas-gen",

		LIBRARIES.vulkan,
	}

	defines
	{
		"GLFW_INCLUDE_NONE",
		"YAML_CPP_STATIC_DEFINE",
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
		defines { "FLARE_RELEASE", "TRACY_ENABLE", "TRACY_IMPORTS" }
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "FLARE_DIST"
		runtime "Release"
		optimize "on"

	filter "configurations:Debug"
		links
		{
			LIBRARIES.shaderc_debug,
			LIBRARIES.spriv_cross_glsl_debug,
			LIBRARIES.spriv_cross_debug,
			LIBRARIES.spriv_tools_debug,

			LIBRARIES.assimp_debug,
			LIBRARIES.assimp_zlib_debug,
		}

	filter "configurations:Release or Dist"
		links
		{
			LIBRARIES.shaderc_release,
			LIBRARIES.spriv_cross_glsl_release,
			LIBRARIES.spriv_cross_release,

			LIBRARIES.assimp_release,
			LIBRARIES.assimp_zlib_release,
		}
