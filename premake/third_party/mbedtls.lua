-------------------------------------------------------------------------------
-- mbedtls.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

third_party_project "mbedtls"
	kind "staticlib"
	includedirs "extern/mbedtls/mbedtls/include"
	files "mbedtls/mbedtls/include/mbedtls/*.h"
	files "mbedtls/mbedtls/library/*.c"

	filter "toolset:msc*"
		disablewarnings "4245" -- signed/unsigned mismatch
		disablewarnings "4127" -- conditional expression is constant
		disablewarnings "4244" -- conversion possible loss of data
		disablewarnings "4310" -- cast truncates constant value
		disablewarnings "4701" -- potentially uninitialized local variable used
		disablewarnings "4132" -- const object should be initialized -- This especially reads as odd, because they're forward declaring a global basically.

	filter "action:*gmake*"
		buildoptions "-U_GNU_SOURCE" -- mbedtls just blindly defines this

	filter "platforms:vader or leia"
		buildoptions "-include unistd_vader.h"
		defines {"__BSD_VISIBLE=1"}
		sysincludedirs("source/adk/steamboat/private/vader/common_compat/sysheaders")
		sysincludedirs("source/adk/steamboat/private/vader/mbedtls_compat/sysheaders")
		files "source/adk/steamboat/private/vader/common_compat/sysheaders/*.c"
		disablewarnings "incompatible-pointer-types" -- incompatible pointer types assigning to 'X *' from 'Y *'
		disablewarnings "implicit-function-declaration" -- implicit declaration of function 'X' is invalid in C99

local m = {}

function m.link()
	filter {KIND_APP}
		links "mbedtls"
end

return m