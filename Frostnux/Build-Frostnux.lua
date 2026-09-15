project "Frostnux"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    targetdir "Binaries/%{cfg.buildcfg}"
    staticruntime "off"
    
    files { "Source/**.h", "Source/**.cpp" }
    
    pchheader ( "fxpch.h" )
    pchsource ( "Source/fxpch.cpp" )
    
    includedirs
    {
        "Source",
        "vendor/GLFW/include",
        "vendor/Glad/include",
        "vendor/glm",
        "vendor/stb_image",
        "vendor/spdlog/include",
		"vendor/json/include",
        "vendor"
    }
    
    defines
    {
        "FX_ENABLE_ASSERTS",
        "GLFW_INCLUDE_NONE"
    }
    
    filter "system:windows"
        links { "GLFW", "Glad", "GLM", "Image", "opengl32.lib" }
        defines { "FX_PLATFORM_WINDOWS" }
        systemversion "latest"
        buildoptions { "/utf-8" }
    
    filter "system:linux"
        buildoptions { "`pkg-config --cflags glfw3`" }
        linkoptions { "`pkg-config --libs glfw3`" }
        links { "GL", "dl" }
        defines { "FX_PLATFORM_LINUX" }
    
    filter {}
    
    targetdir ("%{wks.location}/Binaries/" .. OutputDir)
    objdir ("%{wks.location}/Binaries-Intermediates/" .. OutputDir)
    
    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"
    
    filter "configurations:Release"
        defines { "RELEASE" }
        runtime "Release"
        optimize "On"
        symbols "On"
    
    filter "configurations:Dist"
        defines { "DIST" }
        runtime "Release"
        optimize "On"
        symbols "Off"