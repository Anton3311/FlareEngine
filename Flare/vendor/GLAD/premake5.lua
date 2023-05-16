project "Glad"
    kind "StaticLib"
    language "C"
    staticruntime "off"
    
    targetdir ("bin/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")
    objdir ("bin-int/" .. OUTPUT_DIRECTORY .. "/%{prj.name}")

    files
    {
        "include/glad/glad.h",
        "include/KHR/khrplatform.h",
        "src/glad.c"
    }

    includedirs
    {
        "include"
    }
    
    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"
