project "Flare"
    kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

    files
    {
        "src/**.h",
        "src/**.cpp",
    }

	local flareIncludeDirs = {
        "src",
	}

	for i = 1, #INCLUDE_DIRS do
		flareIncludeDirs[#flareIncludeDirs+1] = INCLUDE_DIRS[i]
	end

    includedirs(flareIncludeDirs)

	links
	{
		"GLAD",
		"GLFW"
	}

	targetdir("%{wks.location}/bin/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")
	objdir("%{wks.location}/bin-int/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")

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
