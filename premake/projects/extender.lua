-------------------------------------------------------------------------------
-- extender.lua
-- Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

project "extender"
	kind "staticlib"
	group "adk"
	files { "**.c", "**.h" }
	removefiles { "generated/extension/ffi.c" }
