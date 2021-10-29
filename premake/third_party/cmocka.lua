-------------------------------------------------------------------------------
-- cmocka.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

third_party_project "cmocka"
		kind "staticlib"
		files "cmocka/src/cmocka.c"
		includedirs "extern/cmocka/include"

		filter "platforms:*win*"
			defines {"HAVE__SNPRINTF_S", "HAVE__VSNPRINTF_S"}
			disablewarnings "4204" -- nonstandard extension used: non-constant aggregate initializer
			disablewarnings "4267" -- conversion from 'X' to 'Y', possible loss of data
			disablewarnings "4701" -- potentially uninitialized local variable 'X' used
			disablewarnings "4703" -- potentially uninitialized local pointer variable 'teardown_name' used
			disablewarnings "4996" -- The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name:
		filter "platforms:*win*"
			defines {
				"HAVE_MALLOC_H",
				"HAVE_INTTYPES_H"
			}
		filter "platforms:vader or leia"
			defines {
				"HAVE__SNPRINTF_S",
				"HAVE__VSNPRINTF_S",
				"HAVE_INTTYPES_H",
				"HAVE_SIGNAL_H"
			}

		filter {"action:gmake*", "toolset:not gcc-4.8-1.7-brcm"}
			disablewarnings "int-conversion"
		filter "action:gmake*"
			buildoptions "-include setjmp.h"
			defines {
				"HAVE_MALLOC_H",
				"HAVE_INTTYPES_H",
				"HAVE_SIGNAL_H"
			}

		filter "toolset:*gcc-*-brcm*"
			disablewarnings "clobbered"

		filter "architecture:x86_64 *64"
			defines "WORDS_SIZEOF_VOID_P=8"
		filter "architecture:x86 or *32"
			defines "WORDS_SIZEOF_VOID_P=4"
