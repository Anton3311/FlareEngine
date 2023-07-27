PROJECT_OUTPUT_DIRECTORY = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/"

local function get_flare_root()
	return _OPTIONS["flare_root"]
end

local function get_project_binary_location(project_name)
	return get_flare_root() .. "/bin/" .. PROJECT_OUTPUT_DIRECTORY .. project_name
end

local function get_library_location (project_name)
	return get_project_binary_location(project_name) .. "/" .. project_name .. ".lib"
end

return {
	setup = function ()
		newoption
		{
			trigger = "flare_root",
			description = "Flare root directory"
		}

		include(get_flare_root() .. "dependencies.lua")
	end,
	get_project_binary_location = get_project_binary_location,
	get_library_location = get_library_location,
	create_workspace = function(workspace_name)
		workspace(workspace_name)
			defines
			{
				"FLARE_BUILD_SHARED"
			}

			architecture "x86_64"

			configurations
			{
				"Debug",
				"Release",
				"Dist",
			}

			flags
			{
				"MultiProcessorCompile"
			}
	end,
	create_project = function(project_name, config_func)
		local dependecies_location = get_flare_root();

		project(project_name)
			kind "SharedLib"
			language "C++"
			cppdialect "C++17"
			staticruntime "off"

			files
			{
				"Assets/**.h",
				"Assets/**.cpp",
			}

			includedirs
			{
				"src/",

				dependecies_location .. "FlareScriptingCore/src/",
				dependecies_location .. "FlareECS/include/",
				dependecies_location .. "FlareCommon/src/",
				dependecies_location .. "Flare/vendor/spdlog/include/",

                dependecies_location .. "Flare/vendor/glm/"
			}

			links
			{
				get_library_location("FlareCommon"),
				get_library_location("FlareScriptingCore"),
			}

			targetdir("%{wks.location}/bin/" .. PROJECT_OUTPUT_DIRECTORY .. "/%{prj.name}")
			objdir("%{wks.location}/bin-int/" .. PROJECT_OUTPUT_DIRECTORY .. "/%{prj.name}")

			filter "system:windows"
				systemversion "latest"
			
			filter "configurations:Debug"
				defines "FLARE_DEBUG"
				runtime "Debug"
				symbols "on"

			filter "configurations:Release"
				defines "FLARE_RELEASE"
				runtime "Release"
				optimize "on"
				
			filter "configurations:Dist"
				defines "FLARE_DIST"
				runtime "Release"
				optimize "on"

		if config_func ~= nil then
			config_func()
		end
	end
}
