-------------------------------------------------------------------------------
-- project.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
---------------------------------------------------------------------------------

if (os.getenv("CARGO_MANIFEST_DIR") ~= nil) or (os.getenv("CI_BUILD") ~= nil) then
	-- msbuild doesn't support gen_projects.bat so suppress it
	return
end

local p = require("premake")

-- handy project for IDEs where we can see all our scripts and assets
io.writefile(ROOT.."/build/premake.timestamp.in", os.date())
io.writefile(ROOT.."/build/premake.timestamp.out", os.date())

-- gather up the arguments/flags we pass to premake before we include any new options (as some of them have defaults)
local premake_invocation_args = ""
for _,arg in pairs(_ARGV) do
	premake_invocation_args = premake_invocation_args.." "..arg
end

io.writefile(ROOT.."/build/gen_projects.bat", "cd ..\r\n"..ROOT.."/premake5.exe"..premake_invocation_args)

local sources = os.match("../../premake/**.lua")

project "adk-m5"
	kind "utility"
	p.original.files("premake/**")
	p.original.files("premake_old/**")
	p.original.files("scripts/**")
	p.original.files("source/shaders/**")
	p.original.files("docs/**")
	p.original.files("docker/**")
	p.original.files("*")
	p.original.removefiles("premake5*")
	p.original.files("build/premake.timestamp.in")
	filter {"files:**.timestamp.in", "action:vs*"}
		buildcommands { "gen_projects.bat" }
		buildoutputs { ROOT.."/build/premake.timestamp.out" }
		buildinputs (sources)
		linkbuildoutputs "off"

local m = {}

-- All projects depend on this one to trigger project updates when things change.

function m.configure(prj)
	if (prj.name ~= "adk-m5") then
		filter "action:vs*"
			dependson "adk-m5"
	end
end

return m
