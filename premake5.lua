workspace "particleSystems"
    configurations {"debug", "release", "test"}
    preferredtoolarchitecture "x86_64"
    startproject "particleSystem"

project "particleSystem"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++latest"
    targetdir "bin/%{cfg.buildcfg}"
    objdir "build/%{cfg.buildcfg}"
    warnings "Extra"
    -- exceptionhandling "Off"
    architecture "x64"

    files {
        "src/**.c*"
    }

    libdirs {
        "/usr/lib/",
        "vendor/SDL/build/%{cfg.buildcfg}",
    }
    links {
        "SDL3",
        "m",
    }

    includedirs {
        "/usr/include/",
        "include/",
        "src/",
        "vendor/SDL/include/"
    }

    filter "configurations:debug"
        defines {"_DEBUG"}
        symbols "On"
        runtime "Debug"
    
    filter "configurations:test"
        buildoptions {"-fsanitize=address"}
        linkoptions {"-fsanitize=address"}
        flags {
            "FatalWarnings",
            "ShadowedVariables",
            "UndefinedIdentifiers",
        }
        runtime "Debug"
        symbols "On"

    filter "configurations:release"
        flags {
            "FatalWarnings",
            "ShadowedVariables",
            "UndefinedIdentifiers",
        }
        optimize "On"
        runtime "Release"
