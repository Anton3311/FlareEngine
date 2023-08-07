project "Flare"
    kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

    files
    {
        "src/**.h",
        "src/**.cpp",

		"vendor/stb_image/stb_image/**.h",
		"vendor/stb_image/stb_image/**.cpp",

		"vendor/ImGuizmo/ImGuizmo.h",
		"vendor/ImGuizmo/ImGuizmo.cpp",
    }

    includedirs
	{
		"src/",
		"%{wks.location}/FlareCommon/src/",
		"%{wks.location}/FlareECS/src/",
		"%{wks.location}/FlareECS/include/",
		"%{wks.location}/FlareScriptingCore/src/",

		INCLUDE_DIRS.GLAD,
		INCLUDE_DIRS.GLFW,
		INCLUDE_DIRS.glm,
		INCLUDE_DIRS.stb_image,
		INCLUDE_DIRS.spdlog,
		INCLUDE_DIRS.imgui,
		INCLUDE_DIRS.imguizmo,
		INCLUDE_DIRS.yaml_cpp,
	}

	links
	{
		"GLAD",
		"GLFW",
		"ImGUI",
		"FlareECS",
		"FlareCommon",
		"FlareScriptingCore",
		"yaml-cpp"
	}

	defines
	{
		"GLFW_INCLUDE_NONE",
		"YAML_CPP_STATIC_DEFINE",
		"FLARE_SCRIPTING_CORE_NO_MACROS",
	}

	targetdir("%{wks.location}/bin/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")
	objdir("%{wks.location}/bin-int/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")

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
