-------------------------------------------------------------------------------
-- steamboat.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

project "steamboat"
	group "adk"

	filter { "options:steamboat-dylib" }
		kind "sharedlib"
	filter { "options:not steamboat-dylib" }
		kind "staticlib"
	filter {}

	files "**.h"
	files "**.c"
	pch()

	includedirs "extern/curl/curl/include"
	removefiles "sb_media_stub.c"

	filter "platforms:*win*"
		links "Iphlpapi"

	filter "platforms:not vader"
		removefiles "private/vader/**"

	filter "platforms:not leia"
		removefiles "private/leia/**"

	filter {"platforms:not leia", "platforms:not vader"}
		removefiles "private/ps/**"

	filter "platforms:vader or leia"
		-- conviva libs
		includedirs {
				"extern/private/conviva/Conviva_SDK_C_2.161.0.37400/src/include",
				"extern/private/conviva/Conviva_SDK_C_2.161.0.37400/src/lib"
		}

		removefiles "ref_ports/**"
		removefiles "restricted/sb_coredump_noop.c"

		files "ref_ports/sb_platform.c"
		files "ref_ports/sb_socket_error_strings.c"
		files "ref_ports/sb_thread.c"

	filter "platforms:not *emu_stb*"
		removefiles "private/display_simulated.*"

	filter {"options:drydock", "platforms:not *stub*"}
		removefiles "**.c"
		files "ref_ports/drydock/*.h"
		files "ref_ports/drydock/*.c"
		files "ref_ports/sb_thread.c"

	filter {"options:not drydock", "platforms:not *stub*"}
		removefiles "ref_ports/drydock/**"

	filter {"platforms:*stub*"}
		removefiles "ref_ports/drydock/**"
		files "ref_ports/drydock/impl_tracking.h"
		files "ref_ports/drydock/impl_tracking.c"

	filter {"platforms:not stb*", "platforms:not *rpi2*", "platforms:not *rpi3*", "platforms:not vader", "platforms:not *mtk*", "platforms:not leia"}
		removefiles "**_gles.*"
		removefiles "**gles_*.*"

	filter {}

	MODULES.strip_platform_files()

local m = {}

function m.link(prj)
	if prj.name ~= "steamboat" then
		filter { KIND_APP }
			links { "steamboat" }
	end
end

return m