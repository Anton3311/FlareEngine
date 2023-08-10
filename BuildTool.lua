local M = {}

local function set_module_defines(module_name, export)
	local module_api_define = string.format("%s_API", string.upper(module_name))

	local import_export = "FLARE_API_IMPORT";
	if export then
		import_export = "FLARE_API_EXPORT"
	end

	local dist_api = string.format("%s=", module_api_define)
	local modular_api = string.format("%s=%s", module_api_define, import_export)

	filter "configurations:not Dist"
		defines { modular_api }

	filter "Dist"
		defines { dist_api }

	filter {}

	targetdir("%{wks.location}/bin/" .. OUTPUT_DIRECTORY)
	objdir("%{wks.location}/bin-int/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")
end

M.define_module = function(name)
	set_module_defines(name, true)
	M.set_module_kind()
end

M.add_module_ref = function(name)
	set_module_defines(name, false)
end

M.set_module_kind = function()
	filter "configurations:Debug or configurations:Release"
		kind "SharedLib"

	filter "configurations:Dist"
		kind "StaticLib"

	filter {}
end

return M
