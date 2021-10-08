-------------------------------------------------------------------------------
-- windows.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
---------------------------------------------------------------------------------

local WINDOWS_7 = "0x0601"
WINVER = WINDOWS_7

local m = {}

function m.strip_platform_files()
	filter "platforms:not *win*"
		removefiles "**_win32.*"
		removefiles "**win32_*.*"
end

if os.target() == "windows" then
	add_default_target "windows"
end

if not target_is_any "windows" then return m end

filter "action:vs*"
	platforms "emu_stb_gpu_win_x86_64"

filter "platforms:*win*"
	defines {
		"WINVER="..WINVER,
		"WIN32_WINNT="..WINVER,
		"WIN32",
		"_CRT_SECURE_NO_WARNINGS",
		'_SB_SYSTEM_METRICS_HEADER="source/adk/steamboat/private/sb_system_metrics_win32.h"'
	}

filter {"platforms:*win*", "configurations:not *ship*"}
	defines "_NATIVE_FFI"

filter "platforms:*win_x86_64"
	defines { "WIN64", "_WIN64" }
	

filter { "platforms:*win*", "kind:consoleapp" }
	entrypoint "mainCRTStartup"

filter { "platforms:*win*", "kind:windowedapp" }
	entrypoint "WinMainCRTStartup"

function m.link()
	filter {"platforms:*win*", KIND_APP}
		links {"opengl32", "glu32"}
end

return m
