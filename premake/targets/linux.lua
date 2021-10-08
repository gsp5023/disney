-------------------------------------------------------------------------------
-- linux.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
---------------------------------------------------------------------------------

local m = {}

if os.target() == "linux" then
	add_default_target "linux"
end

function m.strip_platform_files()
	filter {"platforms:not *deb*"}
		removefiles "**_linux.*"
		removefiles "**linux_*.*"
end

if not target_is_any "linux" then return m end

filter "action:gmake*"
	platforms "emu_stb_gpu_deb_x86_64"

filter "platforms:*deb*"
	defines {"__USE_GNU"}
	defines '_SB_SYSTEM_METRICS_HEADER="source/adk/steamboat/ref_ports/linux/sb_system_metrics_linux.h"'

filter {"platforms:*deb*", "options:coverage"}
	buildoptions { "-fprofile-arcs", "-ftest-coverage" }

filter {"platforms:*deb*", "configurations:not *ship*"}
	defines "_NATIVE_FFI"

function m.link()
	filter { "platforms:*deb*", KIND_APP }
		links {
			"GL",
			"pthread",
			"X11",
			"dl",
			"uuid",
			"m"
		}

	filter { "options:coverage", "platforms:*deb*", KIND_APP }
		links "gcov"
end

return m