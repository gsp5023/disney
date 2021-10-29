-------------------------------------------------------------------------------
-- rpi3.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
-- Add rpi3 flags and settings.
-- compilers and paths are relative to the rpi3:v1 docker image
-------------------------------------------------------------------------------

local m = {}

if not target_is_any {"rpi3_buildroot", "Raspberry Pi3 Build Root"} then return m end

filter "action:gmake*"
	platforms "emu_stb_gpu_rpi3_arm7-a_32"

filter "platforms:*rpi3*"
	defines {"_RPI", "_RPI3", "_BYTE_ORDER_LE", "__USE_GNU", "_USE_DISPMANX"}
	defines {"_SB_SYSTEM_METRICS_HEADER=\"source/adk/steamboat/ref_ports/linux/sb_system_metrics_rpi.h\""}
	makesettings [[ CC = arm-linux-gcc ]]
    makesettings [[ CXX = arm-linux-g++ ]]
	toolset "gcc"
	gccprefix("arm-buildroot-linux-gnueabihf-")

	architecture "arm"
	buildoptions "-march=armv7-a -mtune=cortex-a8 -mfpu=neon -ftree-vectorize"
	buildoptions "-funwind-tables" -- for `backtrace` support

function m.link()
    
	filter {"platforms:*rpi3*", KIND_APP}
		libdirs "/arm-buildroot-linux-gnueabihf_sdk-buildroot/arm-buildroot-linux-gnueabihf/sysroot/usr/lib"
		links {
			"brcmGLESv2",
			"brcmEGL",
			"bcm_host", 
			"vchiq_arm", 
			"vcos",
			"vchostif",
            "pthread",
			"dl",
			"uuid",
			"m", 
            "gstgl-1.0", "gstvideo-1.0", "gstbase-1.0", "gstreamer-1.0", "gobject-2.0", "glib-2.0",
            "gstapp-1.0", "gstbase-1.0", "gstreamer-1.0", "gobject-2.0", "glib-2.0",
            "curl"
		}
end

return m
