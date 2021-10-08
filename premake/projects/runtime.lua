-------------------------------------------------------------------------------
-- runtime.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

project "runtime"
	kind "staticlib"
	group "adk"
	pch()
	files {"**.c", "**.h"}
	includedirs("extern/stb")
	filter "platforms:not *emu_stb*"
		removefiles "private/display_simulated.*"