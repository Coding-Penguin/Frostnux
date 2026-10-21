project "Glad"
	kind "StaticLib"
	language "C"
	staticruntime "off"
	warnings "off"

    targetdir ("%{wks.location}/Binaries/" .. OutputDir)
    objdir ("%{wks.location}/Binaries-Intermediates/" .. OutputDir)

	files
	{
		"**.h",
        "**.c"
	}

	includedirs
	{
		"include"
	}

	filter "system:linux"
		pic "On"

		systemversion "latest"
		
		files
		{
		    "**.h",
            "**.c"
		}

	filter "system:macosx"
		pic "On"

		files
		{
		    "**.h",
            "**.c"
		}

	filter "system:windows"
		systemversion "latest"

		files
		{
		    "**.h",
            "**.c"
		}

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"

	filter { "system:windows", "configurations:Debug-AS" }	
		runtime "Debug"
		symbols "on"
		sanitize { "Address" }

	filter "configurations:Release"
		runtime "Release"
		optimize "speed"

    filter "configurations:Dist"
		runtime "Release"
		optimize "speed"
        symbols "off"