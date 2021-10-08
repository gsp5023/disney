-------------------------------------------------------------------------------
-- paddleboat.lua
-- Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
-- Builds small abstraction layer for loading DSO's and binding their symbols
-------------------------------------------------------------------------------

project ("paddleboat", "source/adk/paddleboat")
	group "launcher"
	kind "staticlib"

	files "**.c"
	files "**.h"

	MODULES.strip_platform_files()
