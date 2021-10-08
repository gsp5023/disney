-------------------------------------------------------------------------------
-- project.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
-- This is the main project generator for the m5 ADK.
-------------------------------------------------------------------------------

KIND_APP = "kind:SharedLib or WindowedApp or ConsoleApp"

local p = require("premake")

local cwd = os.getcwd()
os.chdir(cwd.."/..")
M5_ROOT = os.getcwd()
--print(M5_ROOT)
os.chdir(cwd)

local function main(settings)

	TARGET_DIR = "bin/%{cfg.platform}/%{cfg.buildcfg}"
	BUILD_DIR = os.getenv("CARGO_M5_PREMAKE_OUT_DIR")
	if BUILD_DIR then
		BUILD_DIR = BUILD_DIR.."/build"
	else
		BUILD_DIR = "build"
	end

	settings = settings or {}
	  
	ROOT = settings.root or M5_ROOT
	--print(ROOT)
	PROJECT_NAME = settings.project_name or "adk-m5"
	
	RELPATH = string.sub(M5_ROOT, string.len(ROOT)+2)
	
	if RELPATH ~= "" then
		RELPATH = RELPATH.."/"
	end
	
	--print(RELPATH)
	
	SUBGROUP = nil
	if (settings.generators or {}).projects then
		SUBGROUP = "adk-m5/"
	end
		
	-------------------------------------------------------------------------------
	-------------------------------------------------------------------------------
	-- Project globals
	-------------------------------------------------------------------------------
	-------------------------------------------------------------------------------
	
	WORKSPACE = workspace(PROJECT_NAME:lower())

	startproject "merlin"
	location(BUILD_DIR)
	targetdir(BUILD_DIR.."/"..TARGET_DIR)
	language "c"
	cdialect "c99"
	systemversion "latest"
	exceptionhandling "SEH"
	characterset "ascii"
	warnings "extra"
	flags {"fatalwarnings", "fatallinkwarnings", "multiprocessorcompile"}
	debugdir "."
	staticruntime "off"
	optimize "off"
	defines {'_PROJECT_NAME="'..PROJECT_NAME..'"'}

	-- start in m5 directory
	os.chdir(M5_ROOT)

	-------------------------------------------------------------------------------
	-------------------------------------------------------------------------------
	-- Actions
	-------------------------------------------------------------------------------
	-------------------------------------------------------------------------------
		
	dofile("premake/actions.lua")

	-------------------------------------------------------------------------------
	-------------------------------------------------------------------------------
	-- Configurations
	-------------------------------------------------------------------------------
	-------------------------------------------------------------------------------

	dofile("premake/configs.lua")

	-------------------------------------------------------------------------------
	-------------------------------------------------------------------------------
	-- Load modules
	-- runs all lua project generation code in load_modules()
	-------------------------------------------------------------------------------
	-------------------------------------------------------------------------------

	local modules = require("premake/utils").load_modules(settings.generators)

	if (WORKSPACE.projects) then
		for _,v in pairs(WORKSPACE.projects) do
			p.api.scope.current = v
			modules.optional.configure(v)
		end
		for _,v in pairs(WORKSPACE.projects) do
			p.api.scope.current = v
			if v["tags"]["adk-link-opt-out"] == nil then
				modules.optional.link(v)
			end
		end
	end

	register_targets()
end

return function(settings)
	local call_main = function() 
		main(settings)
	end
	local ok, res = xpcall(call_main, debug.traceback)
	if not ok then
		error(res)
	end
end
