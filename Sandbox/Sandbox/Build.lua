OUTPUT_DIRECTORY = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/"

local build_tool = include(_OPTIONS["flare_root"] .. "BuildTool.lua")

workspace "Sandbox"
	language "C++"
	cppdialect "c++17"
	architecture "x86_64"
	configurations { "Debug", "Release", "Dist" }

	project "Sandbox"
		files { "Source/**.h", "Source/**.cpp" }

	includedirs
	{
		_OPTIONS["flare_root"] .. "Flare/vendor/spdlog/include",
		_OPTIONS["flare_root"] .. "Flare/vendor/glm/",
	}

build_tool.set_module_kind()

build_tool.define_module("Sandbox")
build_tool.add_internal_module_ref("Flare")
build_tool.add_internal_module_ref("FlareCommon")
build_tool.add_internal_module_ref("FlareECS")
