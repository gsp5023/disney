-------------------------------------------------------------------------------
-- json_deflate_tools.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

project ("json_deflate_tool_lib", "source/adk/json_deflate_tool")
	kind "utility"
	group "adk"

	files {"**.h", "**.c"}
	removefiles "private/json_deflate_main.c"
			
	filter_tools_platform {}
		kind "staticlib"

local m = {}

function m.link(prj)
	if prj.name == "tests" then
		filter_tools_platform {}
			links "json_deflate_tool_lib"
	end
end

return m