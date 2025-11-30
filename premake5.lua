workspace "particleSystems"
    configurations {"debug", "release", "test"}
    preferredtoolarchitecture "x86_64"
    startproject "particleSystem"
    -- system "Linux"
    language "C++"
    cppdialect "C++latest"
    targetdir "bin/%{cfg.buildcfg}"
    objdir "build/%{cfg.buildcfg}"
    warnings "Extra"
    architecture "x64"
    
project "particleSystem"
    kind "ConsoleApp"
    exceptionhandling "Off"

    files {
        "src/main.cpp"
    }

    libdirs {
        
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
        defines {"PS_DEBUG"}
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

-- project "test"
--     kind "ConsoleApp"
--     language "C++"
--     cppdialect "C++latest"
--     targetdir "bin/%{cfg.buildcfg}"
--     objdir "build/%{cfg.buildcfg}"
--     warnings "Extra"
--     exceptionhandling "On"
--     architecture "x64"

--     files {
--         "src/test.cpp"
--     }

--     libdirs {
--         "/usr/lib/",
--         "vendor/SDL/build/%{cfg.buildcfg}",
--     }
--     links {
--         "SDL3",
--         "m",
--     }

--     includedirs {
--         "/usr/include/",
--         "include/",
--         "src/",
--         "vendor/SDL/include/"
--     }

--     removeconfigurations {"release", "debug"}
    
--     filter "configurations:test"
--         buildoptions {"-fsanitize=address"}
--         linkoptions {"-fsanitize=address"}
--         flags {
--             "FatalWarnings",
--             "ShadowedVariables",
--             "UndefinedIdentifiers",
--         }
--         runtime "Debug"
--         symbols "On"