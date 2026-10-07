project "Sandbox"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp" }

   includedirs
   {
        "Source",
        "../Frostnux/Source",
        "../Frostnux/vendor/GLFW/include",
        "../Frostnux/vendor/Glad/include",
        "../Frostnux/vendor/glm",
        "../Frostnux/vendor/stb_image",
        "../Frostnux/vendor/spdlog/include",
        "../Frostnux/vendor/nlohmann_json/include",
        "../Frostnux/vendor"
   }

   links { "Frostnux" }

   targetdir ("../Binaries/" .. OutputDir)
   objdir ("../Binaries-Intermediates/" .. OutputDir)

   filter "system:windows"
       systemversion "latest"
       defines { "WINDOWS", "FX_PLATFORM_WINDOWS" }

   filter "system:linux"
       defines { "LINUX", "FX_PLATFORM_LINUX" }

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