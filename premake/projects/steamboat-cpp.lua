-------------------------------------------------------------------------------
-- steamboat-cpp.lua
-- Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

project ("steamboat-cpp", "source/adk/steamboat")
	kind "utility"
	group "adk"
	language "c++"
	cppdialect "c++14"
	pch()
  files "**.h"
  files "**.cpp"

	filter "platforms:not vader"
		removefiles "private/vader/**"

	filter "platforms:not leia"
		removefiles "private/leia/**"

	filter {"platforms:not leia","platforms:not vader"}
		removefiles "private/ps/**"

  MODULES.strip_platform_files()

