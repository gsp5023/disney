-------------------------------------------------------------------------------
-- posix-stub.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
--
-- Overview
--   To support a POSIX based Linux OpenGL ES style Steamboat implementations,
--   offers a simple target and source stub example solution that can be
--   conviniently extended and adapted by a partner.
--
-- General
--   + assumes use of a Linux build host (example Ubuntu LTS 18.04)
--   + assumes build depedency support at path /opt/mw/...:
--      /opt/mw/include   --> header files
--      /opt/mw/lib       --> for shared libraries required for linking
--      /opt/mw/toolchain --> toolchain(s) required for building
--   + for simplicity, assumes minimum 1280x720 capable display use
--   + assumes access to necessary OpenGL ES build headers
--
-- Linux (default, premake option --stubtype=linux):
--   + assumes gcc toolchain support on build host
--   + assumes native X11 support via libX11.so library
--   + assumes native Mesa OpenGL ES 2.0 GPU support via libEGL.so,
--     libGLESv2.so libraries
--
-- mw (premake option --stubtype=mw):
--   + assumes stbgcc-6.3-1.2 toolchain support on build host
--   + assumes access to OpenGL ES 2.0 GPU support via libmwopengl.so library
-------------------------------------------------------------------------------

local m = {}

function m.strip_platform_files()
	filter {"platforms:not *nuuday*"}
		removefiles "**nuuday.*"
end

if not target_is_any {"nuuday"} then return m end

add_gcc_toolset("gcc-6.4.0-rdk-arm32", "/opt/rdk/2.2/sysroots/x86_64-rdksdk-linux/usr/bin/arm-rdk-linux-gnueabi/arm-rdk-linux-gnueabi-") --What is toolset version?

filter "action:gmake*"
	platforms "stb_nuuday_soc_rdkstub_gpu_arm8-a_32"
    	toolset "gcc-6.4.0-rdk-arm32"
    	gccprefix("arm-rdk-linux-gnueabi-")
    	architecture "arm"
 	defines {"BSTD_CPU_ENDIAN=BSTD_ENDIAN_LITTLE"}
    	buildoptions "-march=armv7ve -mthumb -mfpu=neon  -mfloat-abi=hard -mcpu=cortex-a15 -fno-omit-frame-pointer -fno-optimize-sibling-calls -Os -pipe --sysroot=/opt/rdk/2.2/sysroots/cortexa15t2hf-neon-rdk-linux-gnueabi"
	libdirs "/opt/rdk/2.2/sysroots/cortexa15t2hf-neon-rdk-linux-gnueabi/usr/lib" 				-- <-- add path to MW and/or custom libs folder(s), example 'mw/usr/lib'

--filter "platforms:*soc_arm8*"
--	buildoptions "-march=armv8-a+simd+crypto -ftree-vectorize"

filter "platforms:*arm*"
	defines {"_BYTE_ORDER_LE"}

filter "platforms:*rdk*"
	defines {"POSIX_STUB_GLES", "_STB_NATIVE", "__USE_GNU"}
	defines "_SB_SYSTEM_METRICS_HEADER=\"source/adk/steamboat/ref_ports/posix_stub/sb_system_metrics_posix_stub.h\""
	sysincludedirs {
		-- gles
		"/opt/rdk/2.2/sysroots/cortexa15t2hf-neon-rdk-linux-gnueabi/usr/include",	-- <-- PARTNER TODO: add path to GLES headers
		"/opt/rdk/2.2/sysroots/cortexa15t2hf-neon-rdk-linux-gnueabi/usr/include/GLES"		-- <-- PARTNER TODO: add path to GLES headers
	}
        linkoptions {
              "--sysroot=/opt/rdk/2.2/sysroots/cortexa15t2hf-neon-rdk-linux-gnueabi",
              "-Wl,-dynamic-linker,/lib/ld-linux-armhf.so.3"
        }


function m.link()

	filter {"platforms:*rdk*", KIND_APP}
		links "pthread"
		links "dl"
		links "m"
		links {"ocdm", "xkbcommon", "westeros_simpleshell_client", "wayland-client", "wayland-egl", "GLESv2"} 							-- <-- PARTNER TODO: add custom MW and/or vendor OpenGL implementation libraries


	filter "platforms:*rdk*"
		 links {"nghttp2", "ssl", "crypto", "srai", "nxclient", "nexus"}		-- <-- PARTNER TODO: add custom MW and/or SoC/vendor specific libraries needed for Steamboat integration
end

return m
