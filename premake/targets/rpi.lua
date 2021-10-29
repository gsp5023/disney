-------------------------------------------------------------------------------
-- Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

local m = {}

function m.strip_platform_files()
	filter {"platforms:not *rpi1*", "platforms:not *rpi2*", "platforms:not *rpi3*"}
		removefiles "/**dispmanx**"
	filter {"platforms:not *rpi*"}
		removefiles "**rpi**"
end

return m
