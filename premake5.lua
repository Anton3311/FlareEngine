include "dependencies.lua"

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
group ""

group "Core"
	include "FlareCommon"
	include "Flare"
	include "FlareECS"
group ""

group "Editor"
	include "FlareEditor"
group ""

group "Sandbox"
	include "Sandbox"
group ""
