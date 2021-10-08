-------------------------------------------------------------------------------
-- renderer.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

project "renderer"
	kind "staticlib"
	group "adk/renderer"
	files {
		"*.c",
		"*.h",
		"private/*.c",
		"private/*.h"
	}

	pch()
