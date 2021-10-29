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

	define_bundle_key()

	files "**.c"
	files "**.h"

	links "paddleboat"
	links "main"

	includedirs("source/adk/bundle")
	includedirs("extern/cjson")

	build_sym_and_non_sym()

project ("m5", "source/adk/merlin/drivers/minnie")
	group "launcher"
	kind "sharedlib"

	define_bundle_key()

	files "**.c"
	files "**.h"

	filter {"platforms:*win*", "configurations:not *ship*"}
		linkoptions {
			"/INCLUDE:ffi_app_run",
			"/INCLUDE:adk_ffi_exports",
			"/INCLUDE:nve_ffi_exports",
			"/INCLUDE:adk_main",
		}
