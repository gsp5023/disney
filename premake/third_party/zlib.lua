-------------------------------------------------------------------------------
-- zlib.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

third_party_project "m5-zlib"
	kind "staticlib"
	files "zlib/*.h"
	files "zlib/*.c"

	filter "toolset:msc*"
		disablewarnings "4127" -- conditional expression is constant
		disablewarnings "4131" -- uses old-style declarator
		disablewarnings "4244" -- conversion from 'int' to 'short', possible loss of data
		disablewarnings "4245" -- conversion from 'int' to 'unsigned int', signed/unsigned mismatch
		disablewarnings "4267" -- conversion from 'size_t' to 'unsigned int', possible loss of data
		disablewarnings "4324" -- structure was padded due to alignment specifier
		disablewarnings "4996" -- The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name:

	filter "action:gmake*"
		defines { "HAVE_UNISTD_H" }
		disablewarnings "implicit-fallthrough"

	filter { "action:vs*", "platforms:vader or leia" }
		defines { "HAVE_UNISTD_H" }
		disablewarnings "implicit-fallthrough"

local m = {}

function m.link()
	filter {KIND_APP}
		links "m5-zlib"
end

return m