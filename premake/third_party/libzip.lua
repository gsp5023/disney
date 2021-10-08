-------------------------------------------------------------------------------
-- libzip.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
-- Builds libzip subset for use in m5
-------------------------------------------------------------------------------

local function include_libzip()
	includedirs("extern/libzip/lib")
	defines { "ZIP_STATIC" }

	-- Following originally defined by generated libzip config.h, modify as required by compiler capabilities
	defines {
		-- "HAVE__SNPRINTF",
		-- "HAVE__UMASK",
		-- "HAVE_ARC4RANDOM",
		-- "HAVE_CLONEFILE",
		-- undef "HAVE_COMMONCRYPTO",
		-- "HAVE_FICLONERANGE",
		"HAVE_FILENO",
		-- "HAVE_GETPROGNAME",
		-- "HAVE_GNUTLS",
		-- "HAVE_LIBBZ2",
		-- "HAVE_LIBLZMA",
		-- "HAVE_LOCALTIME_R",
		"HAVE_MBEDTLS",
		-- "HAVE_MKSTEMP",
		-- "HAVE_NULLABLE",
		-- "HAVE_OPENSSL",
		"HAVE_SETMODE",
		-- "HAVE_SNPRINTF",
		"HAVE_STRDUP",
		"HAVE_STRTOLL",
		"HAVE_STRTOULL",
		-- "HAVE_STRUCT_TM_TM_ZONE",
		"HAVE_STDBOOL_H",
		-- "HAVE_STRINGS_H",
		-- "HAVE_UNISTD_H",
		-- undef "HAVE_WINDOWS_CRYPTO",
		"SIZEOF_OFF_T=4",
		"SIZEOF_SIZE_T=4",
		-- "HAVE_FTS_H",
		-- "HAVE_NDIR_H",
		-- "HAVE_SYS_NDIR_H",
		-- "WORDS_BIGENDIAN",
		"HAVE_SHARED",
	}
	filter "action:gmake*"
		defines {
			"HAVE_CRYPTO",
			"HAVE_FSEEKO",
			"HAVE_FTELLO",
			"HAVE_UNISTD_H",
		}
	filter { "action:vs*", "platforms:vader or leia" }
		defines { "HAVE_UNISTD_H" }
	filter "platforms:*win*"
		defines {
			"HAVE__CLOSE",
			"HAVE__DUP",
			"HAVE__FDOPEN",
			"HAVE__FILENO",
			"HAVE__SETMODE",
			"HAVE__STRDUP",
			"HAVE__STRICMP",
			"HAVE__STRTOI64",
			"HAVE__STRTOUI64",
			"HAVE__UNLINK",
			"HAVE_STRICMP",
		}
	filter "platforms:not *win*"
		defines{
			"HAVE_STRCASECMP",
			"HAVE_DIRENT_H",
		}
	filter { "platforms:not *win*", "platforms:not leia", "platforms:not vader" }
		defines{
			"HAVE_SYS_DIR_H",
		}
	filter{}
end

third_party_project "m5-libzip"
	kind "staticlib"

	files "libzip/lib/**.h"
	files "libzip/lib/**.c"

	-- TODO check for platform-specific files, prune more unused files

	removefiles "libzip/lib/zip_fdopen.c" -- requires 'dup()' and not needed

	removefiles "libzip/lib/zip_algorithm*"
	files "libzip/lib/zip_algorithm_deflate*"

	removefiles "libzip/lib/zip_crypto*"
	removefiles "libzip/lib/zip_random*"
	files "libzip/lib/zip_crypto_mbedtls*"

	removefiles "libzip/lib/zip_source_file*"
	files "libzip/lib/zip_source_file_common*"

	filter "platforms:*win*"
		files "libzip/lib/zip_source_file_win32*"
		removefiles "libzip/lib/zip_mkstempm.c"
	filter { "platforms:not *win*" }
		files "libzip/lib/zip_source_file_stdio*"
		files "libzip/lib/zip_random_unix.c"
	filter{}

	-- TODO following are an alternative approach, to be deleted
	--files "libzip/lib/zip_source_file_steamboat*"

	include_libzip()

	includedirs("extern/mbedtls/mbedtls/include")
	includedirs("extern/zlib")

	filter "toolset:msc*"
		disablewarnings "4127" -- conditional expression is constant
		disablewarnings "4131" -- uses old-style declarator
		disablewarnings "4189" -- 'temp': local variable is initialized but not referenced
		disablewarnings "4232" -- [TODO fix] nonstandard extension used: 'func': address of dllimport 'func2' is not static, identity not guaranteed
		disablewarnings "4244" -- conversion from 'int' to 'short', possible loss of data
		disablewarnings "4245" -- conversion from 'int' to 'unsigned int', signed/unsigned mismatch
		disablewarnings "4267" -- conversion from 'size_t' to 'unsigned int', possible loss of data
		disablewarnings "4310" -- cast truncates constant value
		disablewarnings "4324" -- structure was padded due to alignment specifier
		disablewarnings "4706" -- assignment within conditional expression
		disablewarnings "4996" -- The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name:

	filter "action:gmake*"
		disablewarnings "implicit-fallthrough"

	filter { "action:vs*", "platforms:vader or leia" }
		disablewarnings "implicit-fallthrough"

local m = {}

m.include_libzip = include_libzip

function m.link()
	filter {KIND_APP}
		links "m5-libzip"
end

return m
