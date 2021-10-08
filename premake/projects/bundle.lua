-------------------------------------------------------------------------------
-- bundle.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
-- Builds bundle file system library
--------------------------------------------------------------------------------

project "bundle"
	group "adk"
	kind "staticlib"
	files {"**.c", "**.h"}
	MODULES.include_libzip()
