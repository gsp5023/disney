-------------------------------------------------------------------------------
-- imagelib.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

project "imagelib"
	kind "staticlib"
	group "adk"
	files {"**.c", "**.h"}
	includedirs("extern/stb")
	