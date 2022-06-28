-------------------------------------------------------------------------------
-- steamboat-media.lua
-- Copyright (c) 2020-2020 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------
-- Builds steamboat-media.a using stubfile. Only builds under specific player and premake option conditions.

if(player_is_any("nve-prebuilt") and _OPTIONS["stub-steamboat-media"]) then
    project ("steamboat-media", "source/adk/steamboat")
        kind "staticlib"
        --header files will be locally sourced in customer repositories
        includedirs "extern/dss-nve/code/steamboat/include"
        includedirs "source/adk/steamboat/nexus"
        includedirs "extern/curl/curl/include"
        includedirs "extern/curl/curl/lib"
        files "**.h"
        files "**.cpp"

        defines {
            "USE_SECURE_PLAYBACK",
            "DIF_SCATTER_GATHER"
        }
end

