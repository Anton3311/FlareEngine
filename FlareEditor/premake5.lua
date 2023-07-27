project "FlareEditor"
    kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

    files
    {
        "src/**.h",
        "src/**.cpp",
    }

    includedirs
    {
        "src",
		"%{wks.location}/Flare/src",
		"%{wks.location}/FlareCommon/src",
		"%{wks.location}/FlareECS/src",
		"%{wks.location}/FlareECS/include",
		"%{wks.location}/FlareScriptingCore/src/",
		INCLUDE_DIRS.glm,
		INCLUDE_DIRS.spdlog,
		INCLUDE_DIRS.imgui,
		INCLUDE_DIRS.yaml_cpp,
    }

	links
	{
		"Flare",
		"ImGUI",
		"FlareCommon",
		"FlareScriptingCore",
		"FlareECS",
		"yaml-cpp",
	}

	defines
	{
		"YAML_CPP_STATIC_DEFINE",
		"FLARE_SCRIPTING_CORE_NO_MACROS",
	}

	debugargs
	{
		"%{wks.location}/Sandbox/Sandbox.flareproj"
	}

	targetdir("%{wks.location}/bin/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")
	objdir("%{wks.location}/bin-int/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")

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

