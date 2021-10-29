-------------------------------------------------------------------------------
-- msc.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

filter "toolset:msc*"
	local cwd = os.getcwd()
	os.chdir(M5_ROOT)
	buildlog (BUILD_DIR.."/logs/%{cfg.buildtarget.basename}/%{cfg.platform}/%{cfg.buildcfg}.log")
	os.chdir(cwd)

	disablewarnings {
		"4100", -- unreferenced format parameter
		"4201", -- nonstandard extension used: nameless struct/union
		"4204", -- nonstandard extension used: non-constant aggregate initializer
		"4210", -- nonstandard extension used: function given file scope
		"4221", -- nonstandard extension used: 'X': cannot be initialized using address of automatic variable 'Y'
		"4505"  -- unreferenced local function has been removed
	}
	
	linkoptions "/ignore:4221" -- This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library

	-- https://docs.microsoft.com/en-us/cpp/build/reference/volatile-volatile-keyword-interpretation?view=vs-2019
	-- we disable MSVC specific volatile behavior in order to enforce
	-- and expose incorrect atomic semantics on windows.
	buildoptions "/volatile:iso"
	staticruntime "off"

filter {"toolset:msc*", KIND_APP, "configurations:*debug*"}
	linkoptions "/STACK:10500000"
filter {"toolset:msc*", KIND_APP, "configurations:not *debug*"}
	linkoptions "/STACK:1500000"
