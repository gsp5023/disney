-------------------------------------------------------------------------------
-- glfw.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

local glfw_platforms = {
	"platforms:*win* or *deb* or *osx* or *rpi4*"
}

local not_glfw_platforms = {
	"platforms:not *win*", 
	"platforms:not *deb*", 
	"platforms:not *osx*", 
	"platforms:not *rpi4*"
}

third_party_project "glfw"
	kind "staticlib"

	filter (glfw_platforms)
		defines "_GLFW_USE_HYBRID_HPG"
		files "glfw/src/**.h"
		files "glfw/src/**.c"
		removefiles "glfw/src/wl_*"
		removefiles "glfw/src/null_*"

	filter "toolset:msc*"
		disablewarnings {
			"4152", -- nonstandard extension, function/data pointer conversion in expression
			"4244", -- conversion from 'const int' to 'char', possible loss of data
			"4456"  -- declaration of 'x' hides previous local declaration
		}

	filter "platforms:*win*"
		defines {"_GLFW_WIN32"}
		removefiles "glfw/src/cocoa_*"
		removefiles "glfw/src/x11_*"
		removefiles "glfw/src/xkb_*"
		removefiles "glfw/src/linux_*"
		removefiles "glfw/src/posix_*"
		removefiles "glfw/src/glx_*"

	filter "platforms:*osx*"
		defines "_GLFW_COCOA"
		files "glfw/src/*.m"
		removefiles "glfw/src/win32_*"
		removefiles "glfw/src/wgl_*"
		removefiles "glfw/src/x11_*"
		removefiles "glfw/src/xkb_*"
		removefiles "glfw/src/linux_*"
		removefiles "glfw/src/glx_*"

	filter "platforms:*deb* or *rpi*"
		defines {"_GLFW_X11", "HAVE_XKBCOMMON_COMPOSE_H"}
		removefiles "glfw/src/cocoa_*"
		removefiles "glfw/src/win32_*"
		removefiles "glfw/src/wgl_*"
		disablewarnings "missing-field-initializers"
		disablewarnings "sign-compare"

	filter (not_glfw_platforms)
		kind "utility"

local m = {}

function m.link_platform(prj)
	if prj.name ~= "glfw" then
		filter(glfw_platforms)
			defines {"_GLFW"}
			includedirs "extern/glfw/include"
		filter(cat(glfw_platforms, KIND_APP))
			links "glfw"
	end
end

function m.strip_platform_files()
	filter(not_glfw_platforms)
		removefiles "**_glfw.*"
		removefiles "**glfw_*.*"
end

return m