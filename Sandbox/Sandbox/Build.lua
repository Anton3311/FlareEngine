OUTPUT_DIRECTORY = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/"

local build_tool = include(_OPTIONS["flare_root"] .. "BuildTool.lua")

build_tool.setup_workspace("Sandbox")
build_tool.setup_project("Sandbox")

build_tool.add_internal_module_ref("Flare")
build_tool.add_internal_module_ref("FlareCommon")
build_tool.add_internal_module_ref("FlareECS")
