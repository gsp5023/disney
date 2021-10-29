-------------------------------------------------------------------------------
-- wasm3.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

third_party_project "wasm3"
	kind "staticlib"
		
	files {
		"wasm3/source/**.h",
		"wasm3/source/**.c",
		"../source/adk/wasm3/**.h",
		"../source/adk/wasm3/**.c"
	}

	-- Tail-call optimization must be enabled for wasm3.
	-- Otherwise, the native stack size requirement is huge.
	optimize "speed"

	filter "configurations:*debug*"
		runtime "debug"
	
	filter "toolset:msc*"
		disablewarnings "4100" -- unused variables
		disablewarnings "4146" -- meaningless operation on unsigned 
		disablewarnings "4189" -- unused variables
		disablewarnings "4201" -- nameless struct/union
		disablewarnings "4204" -- non-constant aggregate initializer
		disablewarnings "4221" -- initialized using address of automatic variable
		disablewarnings "4244" -- possible loss of data
		disablewarnings "4267" -- possible loss of data
		disablewarnings "4702" -- unreachable code

local m = {}

function m.configure()
	defines "_WASM3"
end

function m.link()
	filter {KIND_APP}
		links "wasm3"
end

return m