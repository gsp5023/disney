-------------------------------------------------------------------------------
-- steamboat.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

local p = premake

project "steamboat"
	group "adk"
	tags {"adk-platform"}

	filter { "options:steamboat-dylib" }
		kind "sharedlib"
	filter { "options:not steamboat-dylib" }
		kind "staticlib"
	filter {}

	files "**.h"
	files "**.c"
	pch()

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
				"extern/private/conviva/Conviva_SDK_C_2.176.0.0/src/include",
				"extern/private/conviva/Conviva_SDK_C_2.176.0.0/src/lib"
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

	filter {"platforms:not stb*", "platforms:not *rpi1*", "platforms:not *rpi2*", "platforms:not *rpi3*", "platforms:not vader", "platforms:not *mtk*", "platforms:not leia"}
		removefiles "**_gles.*"
		removefiles "**gles_*.*"

	filter {"platforms:*brcm_bme*"}
		-- Retain the POSIX-y ref-port components, but not the Nexus pieces
		removefiles "ref_ports/nexus/**"

		cppdialect "gnu++0x"

		includedirs "extern/curl/curl/include"
		includedirs "extern/dss-nve/code/third_party/dss-nve-shared/steamboat/include"
		includedirs "extern/private/bme"

		includedirs "source"
		includedirs "source/adk/steamboat"

		p.original.files "extern/private/bme/Display.cpp"
		p.original.files "extern/private/bme/Input.cpp"
		p.original.files "extern/private/bme/Runtime.cpp"
		p.original.files "extern/private/bme/TextToSpeech.cpp"

		if(player_is_any("nve-prebuilt", "nve-shared")) then
			push_filter()

			p.original.files "extern/private/bme/Playback.cpp"
			p.original.files "extern/private/bme/DrmPlayback.cpp"
			p.original.files "extern/private/bme/drm/Drm.cpp"

			filter {"platforms:*brcm_bme*", "tags:bme-prdy-32"}
				p.original.files "extern/private/bme/playready/PlayReadyBase.cpp"
				p.original.files "extern/private/bme/playready/PlayReady3Plus.cpp"
				p.original.files "extern/private/bme/playready/PolicyCallback3Plus.cpp"
				p.original.files "extern/private/bme/playready/Clock.cpp"

			filter {"platforms:*brcm_bme*", "tags:bme-prdy-25"}
				p.original.files "extern/private/bme/playready/PlayReadyBase.cpp"
				p.original.files "extern/private/bme/playready/PlayReady25.cpp"
				p.original.files "extern/private/bme/playready/PolicyCallback25.cpp"

			-- TODO(M5-3316): WV support
			-- p.original.files "extern/private/bme/widevine/Widevine.cpp"

			pop_filter()
		end

	filter {}

	MODULES.strip_platform_files()
