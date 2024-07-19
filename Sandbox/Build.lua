local build_tool = include(_OPTIONS["flare_root"] .. "BuildTool.lua")

build_tool.setup_workspace("Sandbox")
include("Sandbox/Sandbox.Build.lua")
