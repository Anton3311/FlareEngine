OUTPUT_DIRECTORY = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

local root_directory = "../"

include(root_directory .. "dependencies.lua")

workspace "Sandbox"
	defines
	{
		"FLARE_BUILD_SHARED"
	}

	architecture "x86_64"

	configurations
	{
		"Debug",
		"Release",
		"Dist",
	}

	flags
	{
		"MultiProcessorCompile"
	}

	project "Sandbox"
		kind "SharedLib"
		language "C++"
		cppdialect "C++17"
		staticruntime "off"

		files
		{
			"Assets/**.h",
			"Assets/**.cpp",
		}

		includedirs
		{
			"src/",

			root_directory .. "FlareScriptingCore/src/",
			root_directory .. "FlareCommon/src/",
			root_directory .. "Flare/vendor/spdlog/include/"
		}

		links
		{
			"FlareCommon",
			"FlareScriptingCore",
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

	include(root_directory .. "FlareCommon/premake5.lua")
	include(root_directory .. "FlareScriptingCore/premake5.lua")
