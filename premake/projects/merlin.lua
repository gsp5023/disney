-------------------------------------------------------------------------------
-- merlin.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
-- Builds adk app runner (merlin)
-------------------------------------------------------------------------------

newoption {
	trigger = "bundle-key",
	description = "Key used to verify bundle signature (in the form of a base64-encoded byte string)",
}

function define_bundle_key()
	local key = _OPTIONS["bundle-key"]
	if key ~= nil then
		defines { '_ADK_BUNDLE_KEY="' .. key .. '"' }
	end
end

-- Generate 2 targets 
--	* one with symbols named <name>-sym
--	* one without symbols named <name>
-- NOTE: Filtered for _SHIP builds & posix targets
function build_sym_and_non_sym()
	filter{"configurations:*ship", "toolset:gcc*"}
		targetname("%{prj.name}-sym")
		symbols "on"
		postbuildcommands{
			-- strip symbols [targetname]-sym -> [targetname]
			"%{cfg.gccprefix}strip -s %{cfg.targetdir}/%{cfg.buildtarget.name} -o %{cfg.targetdir}/%{prj.name}%{cfg.buildtarget.extension}",
		}
		
	filter{}
end

project ("merlin", "source/adk/merlin")
	group "launcher"
	kind "windowedapp"
	dependson "minnie"

	files "**.c"
	files "**.h"
	links "paddleboat"
	links "main"

	removefiles "drivers/**"
	tags {"adk-link-opt-out"}

	build_sym_and_non_sym()

	-- An alternative to this would be to separate module link() into two
	-- functions: one for "m5" libraries and one for standard platform libraries.
	-- Then we could opt-out of the former and opt-in to the latter...
	filter {"platforms:not vader", "platforms:not leia", "platforms:not *win*"}
		links "dl"

	-- Skip this project for skywalker
	filter {"platforms:vader or leia" }
		kind "utility"

-- Skywalker targets need a separate project because the `tags` field in the
-- merlin project do not respect filters, and Skywalker targets need to statically
-- link to minnie in order to bring in user_memory_hooks. So a separate project
-- is created, with the same output name of `merlin` which statically links to
-- minnie.
-- Tech Debt ticket: https://jira.disneystreaming.com/browse/M5-2004
project ("merlin-skywalker", "source/adk/merlin")
	group "launcher"
	kind "windowedapp"
	links "minnie"
	links "main"
	targetname "merlin"

	-- Skip this project for non-skywalker
	filter {"platforms:not vader", "platforms:not leia"}
		kind "utility"

project ("minnie", "source/adk/merlin/drivers/minnie")
	group "launcher"
	kind "sharedlib"

	-- "/../.." required. Without, premake will set targetdir relative to this script location.
	local cwd = os.getcwd()
	os.chdir(cwd.."/../..")
	targetdir(BUILD_DIR.."/"..TARGET_DIR.."/drivers")
	build_sym_and_non_sym()

	includedirs("source/adk/bundle")
	includedirs("extern/cjson")

	define_bundle_key()

	files "**.c"
	files "**.h"
	targetprefix ""
	targetsuffix ""

	filter_tools_platform()
		links "json_deflate_tool_lib"

	filter {"platforms:leia or vader"}
		kind "staticlib"

project ("m5", "source/adk/merlin/drivers/minnie")
	group "launcher"
	kind "sharedlib"

	define_bundle_key()

	files "**.c"
	files "**.h"

	filter_tools_platform()
		links "json_deflate_tool_lib"

	filter {"platforms:*win*", "configurations:not *ship*"}
		linkoptions {
			"/INCLUDE:ffi_app_run",
			"/INCLUDE:adk_ffi_exports",
			"/INCLUDE:nve_ffi_exports",
			"/INCLUDE:adk_main",
		}
