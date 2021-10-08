-------------------------------------------------------------------------------
-- utils.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

local p = require("premake")
local m = {}
local override_root = nil
local relpath = nil
local console_targets = {}
local IS_WINDOWS = os.target() == "windows"
MODULES = nil

function string:split(sep)
	local fields = {}
	local pattern = (sep and string.format("([^%s]+)", sep)) or "."
	self:gsub(pattern, function(c) fields[#fields + 1] = c end)
	return fields
end

local function dofile(name, ...)
	local chunk, err = loadfile(name)
	if chunk then
		return chunk(...)
	end
	error(err)
end

local filter_stack = {}
local num_filters = 0

local function l_push_filter(terms)
	num_filters = num_filters + 1
	filter_stack[num_filters] = p.configset.getFilter(p.api.scope.current)
	if terms then
		filter(terms)
	end
end

local function l_pop_filter()
	if (num_filters < 1) then
		error "pop_filter: no filter set"
	end
	local terms = filter_stack[num_filters]
	filter_stack[num_filters] = nil
	num_filters = num_filters - 1
	p.configset.setFilter(p.api.scope.current, terms)
end

local function l_apply_filter()
	if (num_filters > 0) then
		local terms = filter_stack[num_filters]
		p.configset.setFilter(p.api.scope.current, terms)
	end
end

local function add_module_components_from_generator(components, generator)
	override_root = ROOT
	local cwd = os.getcwd()
	local funcs = generator()
	for _,v in pairs(funcs) do
		l_push_filter {}
		p.api.scope.current = WORKSPACE
		local c = v()
		if c then
			components[#components + 1] = c
		end
		os.chdir(cwd)
		l_pop_filter()
	end
	override_root = nil
end

local function collect_module_components(paths, generator)

	relpath = RELPATH
	local components = {}
	
	for _,path in pairs(paths) do
		local t = os.match(path)

		for _,v in pairs(t) do
			l_push_filter {}
			p.api.scope.current = WORKSPACE
			local c = dofile(v)
			if c then
				components[#components + 1] = c
			end
			l_pop_filter()
		end
	end

	relpath = ""

	if generator then
		add_module_components_from_generator(components, generator)
	end
	
	return components
end

local function load_module(paths, generator)

	local t = {}
	if type(paths) ~= "table" then
		paths = {paths}
	end

	for _,v in pairs(paths) do
		t[#t + 1] = v.."/*.lua"
		t[#t + 1] = v.."/private/*.lua"
		t[#t + 1] = v.."/restricted/*.lua"
	end

	local components = collect_module_components(t, generator)
	
	local function call(fname, ...) 
		local count = 0
		for k,v in pairs(components) do
			local f = v[fname]
			if f then
				l_push_filter {}				
				f(...)
				l_pop_filter()
				count = count + 1
			end
		end
		return count
	end
	
	local mt = {
		__index = function(t, key) 
			return function(...)
				return call(key, ...)
			end
		end
	}
	
	local t = {}
	setmetatable(t, mt)
	
	return t

end

function m.load_modules(generators)

	filter {}
	
	local function call(key, ...)
		local count = 0
		if MODULES.targets then
			count = count + MODULES.targets[key](...)
		end
		if MODULES.toolsets then
			count = count + MODULES.toolsets[key](...)
		end
		if MODULES.third_party then
			count = count + MODULES.third_party[key](...)
		end
		if MODULES.projects then
			count = count + MODULES.projects[key](...)
		end
		return count
	end

	local noop_mt = {
		__index = function(t, k)
			return function(...) return 0 end
		end
	}

	local noop = {}
	setmetatable(noop, noop_mt)

	MODULES = {
		toolsets = noop,
		targets = noop,
		third_party = noop,
		projects = noop
	}

	local mt = {
		__index = function(t, key)
			return function(...)
				local count = call(key, ...)
				if count < 1 then
					error(key.."() is not implemented in any modules, is the --target valid?")
				end
			end
		end
	}

	setmetatable(MODULES, mt)

	local optional_mt = {
		__index = function(t, key)
			return function(...)
				call(key, ...)
			end
		end
	}

	local optional = {}
	setmetatable(optional, optional_mt)
	MODULES.optional = optional
	
	MODULES.toolsets = load_module("premake/toolsets", (generators or {}).toolsets )

	MODULES.targets = load_module(
		{
			"premake/targets", 
			"premake/targets/brcm",
			"premake/targets/private/brcm",
			"premake/targets/private/partner",
		}, (generators or {}).targets)

	MODULES.third_party = load_module("premake/third_party", (generators or {}).third_party)
	MODULES.projects = load_module("premake/projects", (generators or {}).projects)
		
	return MODULES
end

push_filter = l_push_filter
pop_filter  = l_pop_filter

function push_pop_filter(terms)
	return function(f, ...)
		l_push_filter(terms)
		f(...)
		l_pop_filter()
	end
end

local l_project = project

function third_party_project(name)
	local project = l_project(name)
	project.group = "extern"
	if not override_root then
		project.group = (SUBGROUP or "")..project.group
	end
	project.__path = relpath.."extern"
	project.is_third_party = true
	p.original.includedirs(relpath..".")
end

function project(name, path)
	local project = l_project(name)
	if path then
		if path == "" then
			project.__path = string.sub(relpath, 1, relpath:len()-1)
		else
			project.__path = relpath..path
		end
	else
		project.__path = relpath.."source/adk/"..name
	end
	project.__root = override_root or M5_ROOT
	if not override_root then
		project.group = SUBGROUP
	end
	p.original.includedirs(relpath..".")
end

-- Defines an m5 'extension' project
function extension(name, path)
	project(name, path)

	kind "sharedlib"

	targetprefix ""
	targetsuffix ""

	tags {"adk-link-opt-out"}

	defines {"_SAMPLE_EXTENSION_NAME=\"%{prj.name}\""}

	p.original.files "source/adk/extender/generated/extension/ffi.c"
end

function group(name)
	local project = p.api.scope.project
	if project.is_third_party then
		project.group = "extern/"..name
		if not override_root then
			project.group = (SUBGROUP or "")..project.group
		end
	else
		project.group = name
		if not override_root then
			project.group = (SUBGROUP or "")..project.group
		end
	end
	
end

function pch(path)
	l_push_filter({})
	
	path = (path or p.api.scope.project.__path).."/private/pch"
	local header = path.."/pch.h"
	-- use global RELPATH so we always pull in the m5 pch.cpp
	local source_cpp = RELPATH.."source/pch.cpp"
	local source_c = RELPATH.."source/pch.c"

	p.original.includedirs (path)
	
	defines('_PCH="pch.h"')
	
	local cwd = os.getcwd()
	os.chdir(ROOT)
	
	pchheader("pch.h")
	filter {"language:c"}
		p.original.__files { header, source_c }	
		pchsource(source_c)
	filter {"language:c++"}
		p.original.__files { header, source_cpp }	
		pchsource(source_cpp)

	os.chdir(cwd)
		
	l_pop_filter()
	
end

local function isrelpath(p)
	if IS_WINDOWS then
		local ch2 = p:sub(2,2)
		return ch2 ~= ":"
	end
	local ch1 = p:sub(1,1)
	return ch1 ~= "/"
end

function prefix(path, t)
	local x
	if (type(t) == "table") then
		x = {}
		for k,v in pairs(t) do
			if isrelpath(v) then
				x[k] = path..v
			else
				x[k] = v
			end
		end
	else
		if isrelpath(t) then
			x = path..t
		else
			x = t
		end
	end
	return x
end

p.original = {}
p.original.__files = files
p.original.__removefiles = removefiles
p.original.__includedirs = includedirs
p.original.__libdirs = libdirs
p.original.__syslibdirs = syslibdirs
p.original.__sysincludedirs = sysincludedirs

p.original.files = function (t)
	local cwd = os.getcwd()
	os.chdir(override_root or M5_ROOT)
	p.original.__files(t)
	os.chdir(cwd)
end

p.original.removefiles = function (t)
	local cwd = os.getcwd()
	os.chdir(override_root or M5_ROOT)
	p.original.__removefiles(t)
	os.chdir(cwd)
end

p.original.includedirs = function(t)
	local cwd = os.getcwd()
	os.chdir(override_root or M5_ROOT)
	p.original.__includedirs(t)
	os.chdir(cwd)
end

p.original.libdirs = function(t)
	local cwd = os.getcwd()
	os.chdir(override_root or M5_ROOT)
	p.original.__libdirs(t)
	os.chdir(cwd)
end

p.original.syslibdirs = function(t)
	local cwd = os.getcwd()
	os.chdir(override_root or M5_ROOT)
	p.original.__syslibdirs(t)
	os.chdir(cwd)
end

p.original.sysincludedirs = function(t)
	local cwd = os.getcwd()
	os.chdir(override_root or M5_ROOT)
	p.original.__sysincludedirs(t)
	os.chdir(cwd)
end

function files(t)
	local path = p.api.scope.project.__path
	if path ~= "" then
		t = prefix(path.."/", t)
	end
	p.original.files(t)
end

function removefiles(t)
	local path = p.api.scope.project.__path
	if path ~= "" then
		t = prefix(path.."/", t)
	end
	p.original.removefiles(t)
end

function includedirs(t)
	p.original.includedirs(prefix(relpath, t))
end

function sysincludedirs(t)
	p.original.sysincludedirs(prefix(relpath, t))
end

function libdirs(t)
	p.original.libdirs(prefix(relpath, t))
end

function syslibdirs(t)
	p.original.syslibdirs(prefix(relpath, t))
end

return m
