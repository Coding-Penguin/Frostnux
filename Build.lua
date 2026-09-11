workspace "Frostnux"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "Sandbox"

   -- Workspace-wide build options for MSVC
   filter "system:windows"
      buildoptions { "/EHsc", "/Zc:preprocessor", "/Zc:__cplusplus", "/utf-8" }

OutputDir = "%{cfg.system}-%{cfg.architecture}/%{cfg.buildcfg}"

group "Core"
	include "Frostnux/Build-Frostnux.lua"
group "Application"
   include "Sandbox/Build-Sandbox.lua"
group "Vendor"
   include "Frostnux/vendor/GLFW"
   include "Frostnux/vendor/Glad"
   include "Frostnux/vendor/glm"
   include "Frostnux/vendor/stb_image"
group ""