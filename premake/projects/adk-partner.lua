-------------------------------------------------------------------------------
-- adk-partner.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

project ("adk-partner", "")
    -- set tag to opt out of globally applied links
    --
    tags {"adk-link-opt-out"}

    language "c++"
    symbols "on"
    group "adk"
    
    kind "sharedlib"
    filter { "options:steamboat-dylib" }
        links "steamboat"
    filter {}

local m = {}

function m.link(p)

    -- once v1.0.5 releases: remove "and p.name..."
    if player_is_any("nve-prebuilt") and p.name ~= "steamboat" then
        filter { KIND_APP }
            links "adk-partner"
    end
end

return m

