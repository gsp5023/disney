-------------------------------------------------------------------------------
-- tests.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

project ("tests", "tests")
	kind "consoleapp"
	group "tests"
	files {
		"*.c", "*.h",
		"generated/*.c", "generated/*.h",
	}

	filter { "options:not drydock", "platforms:not *stub*" }
		files { "restricted/*.c", "restricted/*.h" }
	filter {}

	links "cmocka"
	links "bundle"
	links "steamboat"

	includedirs("extern/stb")

	filter "action:gmake*"
		buildoptions "-include setjmp.h"
		links "m"
	filter {}

extension ("test_extension_driven", "tests")
	files "extensions/hello.c"

extension ("test_extension_threaded", "tests")
	files "extensions/hello.c"
