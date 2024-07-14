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
    include "Flare/vendor/GLFW/GLFW.Build.lua"
    include "Flare/vendor/ImGUI/ImGUI.Build.lua"
    include "Flare/vendor/yaml-cpp"
	include "Flare/vendor/msdf/MSDF.Build.lua"
group ""

group "Core"
	include "FlarePlatform/FlarePlatform.Build.lua"
	include "FlareCore/FlareCore.Build.lua"
	include "Flare/Flare.Build.lua"
	include "FlareECS/FlareECS.Build.lua"
group ""

group "Editor"
	include "FlareEditor/FlareEditor.Build.lua"
group ""
