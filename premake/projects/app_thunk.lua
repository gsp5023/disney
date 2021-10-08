-------------------------------------------------------------------------------
-- app_thunk.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

project "app_thunk"
	kind "staticlib"
	group "adk"
	files "**.c"
	files "**.h"

	-- Compile NVE FFI in directly until plugin support
	files { "../nve/ffi/ffi.c" }
