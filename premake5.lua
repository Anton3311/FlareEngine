include "dependencies.lua"

workspace "Flare"
	architecture "x86_64"
	startproject "Sandbox"

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
group ""

group "Core"
	include "Flare"
group ""

group "Sandbox"
	include "Sandbox"
group ""
