-------------------------------------------------------------------------------
-- cjson.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

third_party_project "cjson"
	kind "staticlib"

	files {
		"cjson/**.h",
		"cjson/**.c",
		"../source/adk/cjson/**.h",
		"../source/adk/cjson/**.c"
	}

local m = {}

function m.link()
	filter {KIND_APP}
		links "cjson"
end

return m