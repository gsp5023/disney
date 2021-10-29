-------------------------------------------------------------------------------
-- rpi2.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
-- Add rpi2 flags and settings.
-------------------------------------------------------------------------------

local m = {}

if not target_is_any {"rpi2", "Raspberry Pi2"} then return m end

filter "action:gmake*"
	platforms "emu_stb_gpu_rpi2_arm7-a_32"

filter "platforms:*rpi2*"
	defines {"_RPI", "_RPI2", "_BYTE_ORDER_LE", "__USE_GNU", "_USE_DISPMANX"}
	defines {"_SB_SYSTEM_METRICS_HEADER=\"source/adk/steamboat/ref_ports/linux/sb_system_metrics_rpi.h\""}
	makesettings [[ CC = arm-linux-gnueabihf-gcc ]]
	toolset "gcc"
	gccprefix("arm-linux-gnueabihf-")

	architecture "arm"
	buildoptions "-march=armv7-a -mtune=cortex-a8 -mfpu=neon -ftree-vectorize"
	buildoptions "-funwind-tables" -- for `backtrace` support

	sysincludedirs "/pitools/firmware/opt/vc/include"
	sysincludedirs "/pitools/opt/vc/include"
	sysincludedirs "/pitools/usr/include"

function m.link()
    
	filter {"platforms:*rpi2*", KIND_APP}
		libdirs "/pitools/firmware/opt/vc/lib"
		links {
			"brcmGLESv2",
			"brcmEGL",
			"bcm_host", 
			"vchiq_arm", 
			"vcos",
			"vchostif"
		}

		libdirs "/usr/arm-linux-gnueabihf/lib"

		links {
			"pthread",
			"dl",
			"uuid",
			"m"
		}

		libdirs "/pitools/usr/lib/arm-linux-gnueabihf"
end

return m
