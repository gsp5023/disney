-------------------------------------------------------------------------------
-- osx.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
---------------------------------------------------------------------------------

local m = {}

function m.strip_platform_files()
	filter "platforms:not *osx*"
		removefiles "**_bsd.*"
		removefiles "**bsd_*.*"
		removefiles "**_osx.*"
		removefiles "**osx_*.*"
end

return m