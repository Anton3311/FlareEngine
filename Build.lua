include "Dependencies.lua"

workspace "Flare"
	architecture "x86_64"
	startproject "FlareEditor"

	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	flags
	{
		"MultiProcessorCompile"
	}

OUTPUT_DIRECTORY = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
    include "Flare/vendor/GLFW"
    include "Flare/vendor/GLAD"
    include "Flare/vendor/ImGUI"
    include "Flare/vendor/yaml-cpp"
group ""

group "Core"
	include "FlareCommon/FlareCommon.Build.lua"
	include "Flare/Flare.Build.lua"
	include "FlareECS/FlareECS.Build.lua"
	include "FlareScriptingCore/FlareScriptingCore.Build.lua"
group ""

group "Editor"
	include "FlareEditor/FlareEditor.Build.lua"
group ""
