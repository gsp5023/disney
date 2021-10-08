-------------------------------------------------------------------------------
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

project ("manifest", "source/adk/manifest")
	group "adk"
	kind "staticlib"

	files { "**.h", "**.c" }
