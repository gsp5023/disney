-------------------------------------------------------------------------------
-- json_deflate_tools.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

project ("json_deflate_tool_lib", "source/adk/json_deflate_tool")
	kind "utility"
	group "adk"

	files {"**.h", "**.c"}
	removefiles "private/json_deflate_main.c"
			
	filter_tools_platform {}
		kind "staticlib"

local m = {}

function m.link()
	filter_tools_platform(KIND_APP)
		links "json_deflate_tool_lib"
end

return m
