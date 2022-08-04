-------------------------------------------------------------------------------
-- steamboat-media.lua
-- Copyright (c) 2020-2020 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------
-- Builds steamboat-media.a using stubfile. Only builds under specific player and premake option conditions.

if(player_is_any("nve-prebuilt")) then
    project ("steamboat-media", "source/adk/steamboat")
        kind "staticlib"
        --header files will be locally sourced in customer repositories
        includedirs {
            "/opt/rdk/2.2/sysroots/cortexa15t2hf-neon-rdk-linux-gnueabi/usr/include",
            "/opt/rdk/2.2/sysroots/cortexa15t2hf-neon-rdk-linux-gnueabi/usr/include/opencdm",
            "extern/dss-nve/code/steamboat/include",
            "source/adk/steamboat/nexus",
            "extern/curl/curl/include",
            "extern/curl/curl/lib"
	    }

	    removesysincludedirs "/opt/rdk/2.2/sysroots/cortexa15t2hf-neon-rdk-linux-gnueabi/usr/include"

        files "**.h"
        files "**.cpp"

        defines {
            "USE_SECURE_PLAYBACK"
        }
end

