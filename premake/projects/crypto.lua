-------------------------------------------------------------------------------
-- crypto.lua
-- Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

project ("m5-crypto", "source/adk/crypto")
	kind "staticlib"
	group "adk"
	files { "**.c", "**.h" }