local workspace_name = "Sandbox"

local project_base = include(_OPTIONS["flare_root"] .. "scripting_project_base.lua")
project_base.setup()

project_base.create_workspace(workspace_name)
project_base.create_project("Sandbox")
