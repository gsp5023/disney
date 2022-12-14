-------------------------------------------------------------------------------
-- cmdlets.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

project "cmdlets"
	kind "staticlib"
	group "adk"

	files {
		"cmdlets.c",
		"cmdlets.h",
		"http_test.c",
	}

	filter_tools_platform()
		files {"restricted/**"}
