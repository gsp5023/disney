-------------------------------------------------------------------------------
-- canvas.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

project "canvas"
	kind "staticlib"
	group "adk"
	files {"**.c", "**.h"}
	includedirs("extern/stb")
	
	filter "toolset:msc*"
		disablewarnings "4133" -- 'function': incompatible types - from 'X *' to 'Y *'
