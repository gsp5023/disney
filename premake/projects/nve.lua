-------------------------------------------------------------------------------
-- nve.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
-- nve main project generator
-- contains partner facing configurations
--------------------------------------------------------------------------------

local m = {}

function m.link(p)
    if player_is_any("nve-prebuilt", "nve-shared", "nve-internal") then
        defines "_NVE"
    end

    if player_is_any("nve-prebuilt", "nve-shared") then
        filter "toolset:*gcc*"
            links "stdc++"

        if p.name ~= "nve-api" then
            filter "configurations:debug*"
                libdirs { "extern/dss-nve/stage/debug" }

            filter "configurations:release-o2* or ship*"
                libdirs { "extern/dss-nve/stage/release" }

            filter {}

            libdirs { "build/nve" }
            links "nve-api"
            links "steamboat-media"
        end

	elseif player_is_any("nve-internal") then
		filter { KIND_APP, "platforms:*brcm* or *rpi*" }
			libdirs { "extern/dss-nve/stage/release" }

			links "dss-nve"
			links "steamboat-media"
			links "ssl"
			links "crypto"
			links "nve-api"
			links "stdc++"
    end
end

return m

