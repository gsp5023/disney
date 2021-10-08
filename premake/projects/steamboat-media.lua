-------------------------------------------------------------------------------
-- steamboat-media.lua
-- Copyright (c) 2020-2020 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------
-- Builds steamboat-media.a using stubfile. Only builds under specific player and premake option conditions.

if(player_is_any("nve-prebuilt") and _OPTIONS["stub-steamboat-media"]) then
    project ("steamboat-media", "source/adk/steamboat")
        kind "staticlib"

        cppdialect "c++11"

        --header files will be locally sourced in customer repositories
        includedirs "extern/dss-nve/code/steamboat/include"
        files "**.h"
        files "sb_media_stub.c"
end

