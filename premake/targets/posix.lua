-------------------------------------------------------------------------------
-- linux.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
---------------------------------------------------------------------------------

local m = {}

function m.strip_platform_files()
	filter  { "platforms:*win* or vader or leia" }
		removefiles "**_posix.*"
		removefiles "**posix_*.*"
end

return m