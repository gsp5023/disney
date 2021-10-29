-------------------------------------------------------------------------------
-- Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

local m = {}

if not target_is_any {"rpi1"} then return m end

filter "action:gmake*"
	platforms "emu_stb_gpu_rpi1_arm6-a_32"

filter "platforms:*rpi1*"
	defines {"_RPI", "_RPI1", "_BYTE_ORDER_LE", "__USE_GNU", "_USE_DISPMANX", "_USE_UUID=0"}
	defines {'_SB_SYSTEM_METRICS_HEADER="source/adk/steamboat/ref_ports/linux/sb_system_metrics_rpi.h"'}
	makesettings [[ CC = arm-linux-gnueabihf-gcc ]]
	toolset "gcc"
	gccprefix("arm-linux-gnueabihf-")

	architecture "arm"
	buildoptions "-march=armv6 -mtune=arm1176jzf-s -mfloat-abi=hard -mfpu=vfp"
	buildoptions "-ftree-vectorize"
	buildoptions "-funwind-tables" -- for `backtrace` support

	filter "toolset:*gcc*"
		disablewarnings { "clobbered" }

	sysincludedirs "/opt/firmware/opt/vc/include"

function m.link()
    
	filter {"platforms:*rpi1*", KIND_APP}
		libdirs "/opt/firmware/hardfp/opt/vc/lib/"

		links {
			"brcmGLESv2",
			"brcmEGL",
			"bcm_host", 
			"vchiq_arm", 
			"vcos",
			"vchostif"
		}

		links {
			"pthread",
			"dl",
			"m"
		}
end

return m
