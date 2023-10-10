include("Freetype.Build.lua")

local source_location = "msdf-atlas-gen/"

project "msdfgen"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
    staticruntime "off"

	targetdir ("bin/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")
	objdir ("bin-int/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")

	files
	{
		source_location .. "msdfgen/**.h",
		source_location .. "msdfgen/**.cpp",

		"%{prj.location}/tinyxml2/**.h",
		"%{prj.location}/tinyxml2/**.cpp",
	}

	includedirs
	{
		"%{wks.location}/Flare/vendor/msdf/freetype/include",
		"%{prj.location}/tinyxml2/"
	}

	defines
	{
		"MSDFGEN_USE_CPP11",
		"MSDFGEN_PUBLIC="
	}

	links
	{
		"freetype",
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"
        symbols "off"

	filter ""

project "msdf-atlas-gen"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
    staticruntime "off"

	targetdir ("bin/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")
	objdir ("bin-int/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")


	files
	{
		source_location .. "msdf-atlas-gen/**.h",
		source_location .. "msdf-atlas-gen/**.hpp",
		source_location .. "msdf-atlas-gen/**.cpp",
	}

	includedirs
	{
		source_location .. "msdf-atlas-gen",
		source_location .. "msdfgen",
		source_location .. "msdfgen/include"
	}

	defines
	{
		"_CRT_SECURE_NO_WARNINGS",
		"MSDF_ATLAS_PUBLIC=",
		"MSDF_ATLAS_NO_ARTERY_FONT"
	}

	links
	{
		"msdfgen"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		runtime "Release"
		optimize "on"
        symbols "off"
