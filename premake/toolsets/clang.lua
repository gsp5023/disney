-------------------------------------------------------------------------------
-- clang.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

filter "toolset:*clang*"
	filter {"platforms:not leia", "platforms:not vader", "platforms:not *win*"}
    linkoptions "-fPIC"
  filter "toolset:*clang*"

	disablewarnings {
		"unused-function",
		"unused-parameter",
		"unused-function",
		"missing-braces",
		"missing-field-initializers",
		"unused-variable",
		"unknown-warning-option"
	}

	buildoptions "-Wenum-compare"
	buildoptions "-Wenum-conversion"

filter {"language:c", "toolset:*clang*"}
	buildoptions "-std=gnu99"
