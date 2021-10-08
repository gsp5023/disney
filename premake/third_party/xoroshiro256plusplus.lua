-------------------------------------------------------------------------------
-- xoroshiro256plusplus.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

third_party_project "xoroshiro256plusplus"
    kind "staticlib"
    files {"xoroshiro256plusplus/**.c", "xoroshiro256plusplus/**.h"}

local m = {}

function m.link()
    filter {KIND_APP}
	    links "xoroshiro256plusplus"
end

return m