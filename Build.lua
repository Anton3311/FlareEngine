include "Dependencies.lua"
include "BuildTool.lua"

workspace "Flare"
	architecture "x86_64"
	startproject "FlareEditor"

	configurations
	{
		"Debug",
		"Release",
		"Dist",
	}

	filter "configurations:not Dist"
		disablewarnings
		{
			"4251"
		}

	flags
	{
		"MultiProcessorCompile"
	}

group "Dependencies"
    include "Flare/vendor/GLFW"
    include "Flare/vendor/GLAD"
    include "Flare/vendor/ImGUI"
    include "Flare/vendor/yaml-cpp"
group ""

group "Core"
	include "FlarePlatform/FlarePlatform.Build.lua"
	include "FlareCommon/FlareCommon.Build.lua"
	include "Flare/Flare.Build.lua"
	include "FlareECS/FlareECS.Build.lua"
group ""

group "Editor"
	include "FlareEditor/FlareEditor.Build.lua"
group ""
