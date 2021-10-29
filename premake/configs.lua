-------------------------------------------------------------------------------
-- configs.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

-- Configurations
configurations {"debug", "release-o2", "ship"}

local console_targets = {"vader", "leia", "luke"}

function cat(t, ...)
	local new = {table.unpack(t)}

	for i,v in ipairs({...}) do
		if type(v) == "table" then
			for ii,vv in ipairs(v) do
				new[#new + 1] = vv
			end
		else
			new[#new + 1] = v
		end
	end
	return new
end

function filter_not_console(...)

	local t = {}
	for _,v in pairs(console_targets) do
		t[#t] = "platforms:not "..v
	end

	filter(cat(t, ...))
end

function filter_console(...)

	local s = ""
	for _,v in pairs(console_targets) do
		if s ~= "" then
			s = s.." or "..v
		else
			s = "platforms:"..v
		end
	end

	filter(cat({s}, ...))
end


function filter_tools_platform(...)
	filter(cat({"platforms:*deb* or *win*"}, ...))
end

function filter_not_tools_platform(...)
	filter(cat({"platforms:not *deb*", "platforms:not *win*"}, ...))
end

defines {
	"CURL_STATICLIB"
}

filter "action:gmake*"
	linkgroups "On"

filter "platforms:*stb*"
	defines "_STB"

filter {"platforms:*stb* or *mtk*","platforms:not *emu_stb*"}
	defines "_STB_NATIVE"

filter "platforms:*gpu*"
	defines "_GPU"

filter_console {}
	-- for now all console targets are native, but like STBs we
	-- may one-day want to provide a desktop "emu" version.
	defines {"_CONSOLE_NATIVE"}

filter "platforms:*x86_64"
	architecture "x86_64"
	defines {"_BYTE_ORDER_LE"}

filter "platforms:*x86"
	architecture "x86"
	defines {"_BYTE_ORDER_LE"}

filter {"options:drydock"}
	defines {"_DRYDOCK"}

filter "configurations:*debug"
	defines {"_DEBUG"}
	symbols "full"
	editandcontinue "on"
	runtime "debug"
	optimize "off"

filter "configurations:*release-o2"
	defines {"NDEBUG"}
	symbols "on"
	editandcontinue "off"
	optimize "speed"
	flags {"noincrementallink", "nominimalrebuild"}

filter "configurations:*ship"
	defines {"NDEBUG", "_SHIP"}
	symbols "off"
	editandcontinue "off"
	optimize "speed"
	flags {"noincrementallink", "nominimalrebuild"}

if not player_is_any("nve-partner") then
	filter {"configurations:*release-o2 or *ship", "toolset:not *gcc-*-brcm*", "platforms:not *rpi1*"}
		flags {"linktimeoptimization"}
end

filter {"configurations:*telemetry*"}
	defines {"_TELEMETRY"}

filter {"options:canvas-experimental"}
	defines {"_CANVAS_EXPERIMENTAL"}

filter {}
