-------------------------------------------------------------------------------
-- rpi4.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
-- Add rpi4 flags and settings.
-------------------------------------------------------------------------------

-- NOTES: you must run raspi-config and change the GL drivers to the experimental KLM desktop drivers
-- otherwise framerate suffers.

local m = {}

if not target_is_any {"rpi4", "Raspberry Pi4"} then return m end

filter "action:gmake*"
	platforms "emu_stb_gpu_rpi4_arm7-a_32"

filter "platforms:*rpi4*"
	defines {"_RPI", "_RPI4", "_BYTE_ORDER_LE", "__USE_GNU"}
	defines {"_SB_SYSTEM_METRICS_HEADER=\"source/adk/steamboat/ref_ports/linux/sb_system_metrics_rpi.h\""}
	makesettings [[ CC = arm-linux-gnueabihf-gcc ]]
	toolset "gcc"
	gccprefix("arm-linux-gnueabihf-")

	architecture "arm"
	buildoptions "-march=armv7-a -mtune=cortex-a8 -mfpu=neon -ftree-vectorize"
	buildoptions "-funwind-tables" -- for `backtrace` support

	sysincludedirs "/pitools/firmware/opt/vc/include"
	sysincludedirs "/pitools/usr/include"
	
function m.link()

	filter {"platforms:*rpi4*", KIND_APP}
		libdirs "/pitools/firmware/opt/vc/lib"
		libdirs "/usr/arm-linux-gnueabihf/lib"
		libdirs "/pitools/usr/lib/arm-linux-gnueabihf"

		links {
			"GL", 
			"bcm_host", 
			"vchiq_arm", 
			"vcos", 
			"GLX", 
			"xcb", 
			"Xau", 
			"Xdmcp",
			"pthread",
			"X11",
			"dl",
			"uuid",
			"m"
		}
		
		linkoptions {
			"-l:libGLdispatch.so.0", 
			"-l:libbsd.so.0",
			"-Wl,-rpath-link=/pitools/usr/lib/arm-linux-gnueabihf",
		}

end

return m
