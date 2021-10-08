-------------------------------------------------------------------------------
-- actions.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
---------------------------------------------------------------------------------

local p = require("premake")

newoption {
	trigger = "drydock",
	description = "Build steamboat with empty (no-op) stubs"
}


newoption {
	trigger = "stub-steamboat-media",
	description = "Build steamboat-media with empty (no-op) stubs (player=nve-prebuilt only)"
}

newoption {
	trigger = "player",
	default = "null",
	description = "Video player backend",
	value="VALUE",
	allowed = {
		{ "null", "no-op player integration (default)" },
		{ "nve-prebuilt", "Link to (externally) pre-built DSS NVE player" },
		{ "nve-partner", "Builds DSS NVE player for sharing with partners" },
		{ "nve-internal", "DSS NVE player integration (deprecated, use nve-shared instead)" },
		{ "nve-shared", "DSS NVE player integration (dynamically linked)" },
	}
}

newoption {
	trigger = "drm_pr_version",
	default = "25",
	description = "PlayReady Version",
	value="VALUE",
	allowed = {
		{ "25", "PlayReady 2.5" },
		{ "30", "PlayReady 3.0" },
	}
}

newoption {
	trigger = "target",
	value = "VALUE",
	description = "Comma-separated list of the targeted platforms",
}

newoption {
	trigger = "coverage",
	description = "Enables code-coverage compiler hooks",
}

newoption {
	trigger = "research",
	description = "Enables the inclusion of research projects",
}

newoption {
	trigger = "steamboat-dylib",
	description = "Enables compilation of steamboat as a dynamic (shared) library",
}

-- match strings based on '*' wildcard
-- can do things like player_is_any("nve*") or player_is_nay("nve-prebuilt*") etc
-- can do things like target_is_any("brcm*")

function match_wildcards(x, z)
	-- convert special characters into escaped versions
	-- so they are ignored except for * which we translate
	-- into a real wildcard
	local has_wildcards = false
	local s = string.gsub(x, "[%[%(%)%.%%%+%-%*%?%[%^%$%]]", function(y)
		if (y == "*") then
			has_wildcards = true
			return ".-"	
		end
		return "%"..y
	end)

	if (has_wildcards) then
		local r = string.gmatch(z, s)() ~= nil
		return r
	end
	
	return x == z
end

function player_is_any(...)
	if (_OPTIONS["player"] == nil) or (_OPTIONS["player"] == "null") then
		return false
	end

	local x = {...}

	if #x == 0 then
		return true
	end

	local t = {}

	for _,v in pairs(x) do
		if type(v) == "table" then
			for _,z in pairs(v) do
				t[#t + 1] = z
			end
		else
			t[#t + 1] = v
		end
	end
	
	for _,v in pairs(t) do
		if match_wildcards(v, _OPTIONS["player"]) then
			return true
		end
	end

	return false
end

function is_playready_drm_version(version)
	if (_OPTIONS["drm_pr_version"] == nil) then
		return false
	end

	return version == _OPTIONS["drm_pr_version"]
end

function is_playready_drm_version_30()
	return is_playready_drm_version("30")
end

function is_playready_drm_version_25()
	return is_playready_drm_version("25")
end

local target_list = {{"all", "enable all targets"}}
local default_targets = ""
local target_default = true

for _,arg in pairs(_ARGV) do
	local i = arg:find("=", 1, true)
	local key
	if i then
		key = arg:sub(1, i - 1)
	else
		key = arg
	end
	if key == "--target" then
		target_default = false
	end
end

function add_target(t)
	local name = t
	if type(t) == "table" then
		name = t[1]
	end

	if string.find(name, "*") == nil then
		local found = false
		for _,v in pairs(target_list) do
			found = (v[1] == name)
			if found then
				break
			end
		end
		if not found then
			if type(t) == "table" then
				target_list[#target_list + 1] = {t[1], t[2]}
			else
				target_list[#target_list + 1] = {t, ""}
			end
		end
	end
end

function add_default_target(t)
	add_target(t)
	local list = default_targets:split(",")
	local name = t
	if type(t) == "table" then
		name = t[1]
	end

	local found = false
	for _,v in pairs(list) do
		if v == name then
			found = true
			break
		end
	end
	if not found then
		if default_targets == "" then
			default_targets = name
		else
			default_targets = default_targets..","..name
		end
	end
end

function target_is_any(...)
	local x = {...}
	local t = {}

	for _,v in pairs(x) do
		if type(v) == "table" then
			t[#t + 1] = {v[1], v[2]}
		else
			t[#t + 1] = v
		end
	end
	
	for _,v in pairs(t) do
		add_target(v)
	end

	if _OPTIONS["target"] == "all" then
		return true
	end

	local targets = default_targets
	if not target_default then
		targets = _OPTIONS["target"]
	end
	targets = targets:split(",")

	for _,v in pairs(t) do
		local name = v
		if type(v) == "table" then
			name = v[1]
		end

		for _, value in pairs(targets) do
			if match_wildcards(name, value) then
				return true
			end
		end
	end

	return false
end

-- this gathers all registered targets at the end
-- of running all the premake configuration and
-- does a couple things
-- 1) if --help is specified it modifies the premake action
--    in order to get pretty printing of the --target list
--    we can't depend on premakes default option validation because
--    we want --target to support a comma delimited list
-- 2) if no --help is specified then we make sure we either have
--    default targets (if no --target was specified) or we make sure
--    that the --target list are all valid

function register_targets()
	local help = false;
	for _,v in pairs(_ARGV) do
		if v == "--help" then
			help = true;
			break
		end
	end

	local player_str = ""

	if player_is_any() then
		player_str = " with --player=".._OPTIONS["player"]
	end

	if help then
		local option = p.option.list["target"]
		option.default = default_targets
		option.allowed = target_list
	elseif target_default then
		if default_targets == "" then
			error "no default target(s) set"
		end
		print("Configuring "..default_targets..player_str.."...")
	else
		print("Configuring ".._OPTIONS["target"]..player_str.."...")
		if _OPTIONS["target"] ~= "all" then
			-- validate the provided options
			local targets = _OPTIONS["target"]:split(",")
			for _,v in pairs(targets) do
				found = false
				for _,x in pairs(target_list) do
					if x[1] == v then
						found = true
						break
					end
				end
				if not found then
					error("'"..v.."' is not a valid target")
				end
			end
		end
	
	end
end
