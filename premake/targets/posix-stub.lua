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

newoption {
	trigger = "stubtype",
	default = "linux",
	description = "stub build type indicator",
	allowed = {
		{ "linux", "stub based on Linux + X11/OpenGL ES (default)" },
		{ "mw", "stub based on MW + OpenGL ES" },
	}
}

local m = {}

function m.strip_platform_files()
	filter {"platforms:not *stub*"}
		removefiles "**_posix_stub.*"
		removefiles "**posix_stub_*.*"
end

if not target_is_any {"stub", "Posix/OpenGL Steamboat example"} then return m end

local function is_stubtype_linux()
	return _OPTIONS["stubtype"] == "linux"
end

local function is_stubtype_mw()
	return _OPTIONS["stubtype"] == "mw"
end

if is_stubtype_mw() then
	add_gcc_toolset("gcc-6.3-1.2-soc", "/opt/mw/toolchain/stbgcc-6.3-1.2/bin/aarch64-linux-")
end

filter "action:gmake*"
	if is_stubtype_linux() then
		platforms "stb_emu_gpu_mwstub_x86_64"
		toolset "gcc"
		architecture "x86_64"
		libdirs "/opt/mw/lib/linux"				-- <-- add path to MW and/or custom libs folder(s), example 'mw/usr/lib'
	end
	if is_stubtype_mw() then
		platforms "stb_soc_gpu_mwstub_arm8-a_64"
		toolset "gcc-6.3-1.2-soc"
		architecture "arm64"
		libdirs "/opt/mw/lib/mw" 				-- <-- add path to MW and/or custom libs folder(s), example 'mw/usr/lib'
	end

filter "platforms:*soc_arm8*"
	buildoptions "-march=armv8-a+simd+crypto -ftree-vectorize"

filter "platforms:*arm*"
	defines {"_BYTE_ORDER_LE"}

filter "platforms:*x86*"
	defines {"MWSTUB_EXAMPLE_X11_OPENGLES"}

filter "platforms:*stub*"
	defines {"POSIX_STUB_GLES", "_STB_NATIVE", "__USE_GNU"}
	defines "_SB_SYSTEM_METRICS_HEADER=\"source/adk/steamboat/ref_ports/posix_stub/sb_system_metrics_posix_stub.h\""

filter "platforms:*x86*"
	sysincludedirs {
		-- gles
		"/opt/mw/include/gles/linux/include"	-- <-- PARTNER TODO: add path to GLES headers
	}

filter "platforms:*mw*"
	sysincludedirs {
		-- gles
		"/opt/mw/include/gles/mw/include"		-- <-- PARTNER TODO: add path to GLES headers
	}

	includedirs {
		"/opt/mw/include/"						-- <-- PARTNER TODO: add path to device support and/or MW headers
	}

function m.link()
	filter {"platforms:*stub*", KIND_APP}
		links "pthread"
		links "dl"
		links "m"

	filter {"platforms:*soc*", KIND_APP}
		links {"mwopengl"} 							-- <-- PARTNER TODO: add custom MW and/or vendor OpenGL implementation libraries

	filter {"platforms:*x86*", KIND_APP}
		links {"X11","EGL","GLESv2"} 

	filter "platforms:*mw*"
		-- links {"soclibrary","mwlibrary",...}		-- <-- PARTNER TODO: add custom MW and/or SoC/vendor specific libraries needed for Steamboat integration
end

return m