-------------------------------------------------------------------------------
-- libwebsockets.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
-- Builds libwebsockets for use in m5
-------------------------------------------------------------------------------

local function config_websockets()
	defines {
		"LWS_HAVE_INTTYPES_H",
		"LWS_INSTALL_DATADIR=\"build/lws_datadir\"", -- hopefully just for symbol completion, has to do with vhosts see libwebsockets/lib/roles/http/server/lejp-conf.c: 890
		"LWS_LIBRARY_VERSION_MAJOR=4",
		"LWS_LIBRARY_VERSION_MINOR=0",
		"LWS_LIBRARY_VERSION_PATCH=99",
		"LWS_LIBRARY_VERSION_NUMBER=(LWS_LIBRARY_VERSION_MAJOR * 1000000) + (LWS_LIBRARY_VERSION_MINOR * 1000) + LWS_LIBRARY_VERSION_PATCH",
		"LWS_MAX_SMP=1",
		"LWS_BUILD_HASH=\"v4.0.0-72-g3779d9be\"",
		"LWS_CLIENT_HTTP_PROXYING",
		"LWS_HAS_INTPTR_T",
		"LWS_HAS_GETOPT_LONG",
		"LWS_HAVE_ATOLL",
		"LWS_HAVE_mbedtls_md_setup",
		"LWS_HAVE_mbedtls_net_init",
		"LWS_HAVE_mbedtls_rsa_complete",
		"LWS_HAVE_mbedtls_internal_aes_encrypt",
		"LWS_HAVE_RSA_SET0_KEY",
		"LWS_HAVE_SSL_CTX_get0_certificate",
		"LWS_HAVE_SSL_CTX_set1_param",
		"LWS_HAVE_SSL_CTX_set_ciphersuites",
		"LWS_HAVE_SSL_EXTRA_CHAIN_CERTS",
		"LWS_HAVE_SSL_get0_alpn_selected",
		"LWS_HAVE_SSL_CTX_EVP_PKEY_new_raw_private_key",
		"LWS_HAVE_SSL_set_alpn_protos",
		"LWS_HAVE_SSL_SET_INFO_CALLBACK",
		"LWS_HAVE_STDINT_H",
		"LWS_HAVE_TLS_CLIENT_METHOD",
		"LWS_HAVE_TLSV1_2_CLIENT_METHOD",
		"LWS_HAVE_X509_get_key_usage",
		"LWS_HAVE_X509_VERIFY_PARAM_set1_host",
		"LWS_LIBRARY_VERSION=\"4.0.99\"",
		"LWS_OPENSSL_SUPPORT",
		"LWS_ROLE_H1",
		"LWS_ROLE_H2",
		"LWS_ROLE_RAW",
		"LWS_ROLE_RAW_FILE",
		"LWS_ROLE_WS",
		"LWS_SSL_CLIENT_USE_OS_CA_CERTS",
		"LWS_WITH_CUSTOM_HEADERS",
		"LWS_WITH_DIR",
		"LWS_WITH_FILE_OPS",
		"LWS_WITH_HTTP2",
		"LWS_WITH_HTTP_BASIC_AUTH",
		"LWS_WITH_HTTP_UNCOMMON_HEADERS",
		"LWS_WITH_LEJP",
		"LWS_WITH_LWSAC",
		--"LWS_LOGS_TIMESTAMP",
		"LWS_WITH_NETWORK",
		"LWS_WITH_CLIENT",
		"LWS_WITHOUT_EXTENSIONS",
		"LWS_WITH_SERVER",
		"LWS_WITH_POLL",
		"LWS_WITH_SEQUENCER",
		"LWS_WITH_TLS",
		"LWS_WITH_UDP",
		"LWS_WITH_MBEDTLS",
		"LWS_WITH_SOCKS5",
		"LWS_HAVE_mbedtls_ssl_get_alpn_protocol",
	}

	filter "action:gmake*"
		defines {
			"LWS_PLAT_UNIX",
			"LWS_HAVE_PTHREAD_H",
			"LWS_NO_DAEMONIZE",
			"LWS_HAVE_PIPE2",
			"LWS_HAVE_CLOCK_GETTIME",
			"LWS_HAVE_MALLOC_H",
			"LWS_HAVE_MALLOC_TRIM",
		}

	filter { "platforms:*win*" }
		defines {
			"LWS_PLAT_WINDOWS",
			"LWS_HAVE__STAT32I64",
		}

	filter { "platforms:vader or leia" }
		buildoptions "-include unistd_vader.h"
		defines {
			"LWS_PLAT_UNIX",
			"LWS_NO_DAEMONIZE",
			"LWS_SOMAXCONN=64",
			"LWS_AVOID_SIGPIPE_IGN",
		}
	filter{}

	includedirs "extern/libwebsockets"
	includedirs "extern/libwebsockets/libwebsockets/include"
	includedirs "extern/libwebsockets/libwebsockets/lib"
	includedirs "extern/libwebsockets/libwebsockets/lib/core"
	includedirs "extern/libwebsockets/libwebsockets/lib/roles"
	includedirs "extern/libwebsockets/libwebsockets/lib/roles/http"
	includedirs "extern/libwebsockets/libwebsockets/lib/roles/h1"
	includedirs "extern/libwebsockets/libwebsockets/lib/roles/h2"
	includedirs "extern/libwebsockets/libwebsockets/lib/roles/ws"
	includedirs "extern/libwebsockets/libwebsockets/lib/tls"
	includedirs "extern/libwebsockets/libwebsockets/lib/event-libs"
	includedirs "extern/libwebsockets/libwebsockets/lib/core-net"
	includedirs "extern/libwebsockets/libwebsockets/lib/event-libs/poll"
	includedirs "extern/libwebsockets/libwebsockets/lib/abstract"
	includedirs "extern/libwebsockets/libwebsockets/lib/jose/jwe"
	includedirs "extern/libwebsockets/libwebsockets/lib/jose"

	filter { "platforms:*win*" }
		includedirs "extern/libwebsockets/libwebsockets/lib/plat/windows"
		includedirs "extern/libwebsockets/libwebsockets/win32port/win32helpers"

	filter "action:gmake*"
		includedirs "extern/libwebsockets/libwebsockets/lib/plat/unix"

	filter { "platforms:vader or leia" }
    	includedirs "extern/libwebsockets/libwebsockets/lib/plat/unix"
	
	filter{}

	includedirs "extern/libwebsockets/libwebsockets/lib/tls/mbedtls/wrapper/include/internal"
	includedirs "extern/libwebsockets/libwebsockets/lib/tls/mbedtls/wrapper/include"
	includedirs "extern/libwebsockets/libwebsockets/lib/tls/mbedtls/wrapper/include/platform"
	includedirs "extern/mbedtls/mbedtls/include"
	includedirs "extern/zlib"
end

third_party_project "libwebsockets"
	kind "staticlib"
		
	config_websockets()

	local original = {
		files = files,
		removefiles = removefiles
	}

	filter "platforms:*win*"
		files "libwebsockets/libwebsockets/win32port/win32helpers/**"
	filter{}

	local files = function(...)
		original.files(prefix("libwebsockets/libwebsockets/lib/", ...))
	end

	local removefiles = function(...)
		original.removefiles(prefix("libwebsockets/libwebsockets/lib/", ...))
	end

	original.files("libwebsockets/lws_config.h")
	original.files("libwebsockets/libwebsockets/include/**.h")

	files {"**.h", "**.c"}
	filter {"platforms:not *deb*", "platforms:not *brcm*", "platforms:not *rpi*", "platforms:not vader", "platforms:not leia", "platforms:not *stub*","platforms:not *mtk*"}
		removefiles "plat/unix/**"
	filter "platforms:not *win*"
		removefiles "plat/windows/**"
	filter{}

	filter{"platforms:not *win*"}
		buildoptions "-fvisibility=hidden"
	filter{}

	removefiles "plat/freertos/**"
	removefiles "plat/optee/**"
	removefiles "plat/unix/android/**"

	removefiles "event-libs/glib/**"
	removefiles "event-libs/libev/**"
	removefiles "event-libs/libuv/**"
	removefiles "event-libs/libevent/**"
	removefiles "abstract/**"

	--removefiles "misc/**"
	removefiles "misc/getifaddrs.*"
	removefiles "misc/diskcache.*"
	removefiles "misc/threadpool/**"
	removefiles "misc/daemonize*"
	removefiles "misc/*sqlite*"
	removefiles "misc/peer-limits*"
	removefiles "misc/fsmount*"

	removefiles "secure-streams/**"
	removefiles "tls/openssl/**"
	removefiles "system/dhcpclient/**"
	removefiles "system/async-dns/**"

	removefiles "roles/dbus/**"
	removefiles "roles/cgi/**"
	removefiles "roles/http/compression/**"
	--removefiles "roles/http/server/**"
	removefiles "roles/http/server/ranges*"
	removefiles "roles/http/server/access-log*"

	removefiles "roles/mqtt/**"
	removefiles "roles/ws/ext/**"
	removefiles "roles/raw-proxy/**"

	removefiles "core-net/detailed-latency.c"
	removefiles "core/lws_dll.c"
	removefiles "roles/h2/minihuf.c"
	removefiles "core-net/stats.c"
	removefiles "roles/http/minilex.c"
	removefiles "core-net/server.c"
	removefiles "plat/windows/*spawn*"
	removefiles "plat/unix/*spawn*"
	removefiles "plat/windows/*plugins*"
	removefiles "plat/unix/*plugins*"
	removefiles "roles/http/server/rewrite*"

	-- remove warnings
	filter "toolset:msc*"
		disablewarnings "4214" -- nonstandard extension used: bit field types other than int
		disablewarnings "4100" -- unreferenced formal parameter
		disablewarnings "4244" -- conversion from <const int> to <unsigned>
		disablewarnings "4245" -- signed/unsigned mismatch
		disablewarnings "4456" -- declaration hides previous local declaration
		disablewarnings "4311" -- type cast pointer truncation
		disablewarnings "4127" -- conditional expression is constant
		disablewarnings "4996" -- """"the posix name is deprecated"""" (sure thing there)
		disablewarnings "4701" -- potentially uninit variable used
		disablewarnings "4090" -- function different const qualifiers
		disablewarnings "4389" -- '==' signed/unsigned mismatch
		disablewarnings "4702" -- unreachable code
		disablewarnings "4703" -- potentially unitialized variable used
		disablewarnings "4267" -- conversion (x to y) possible loss of data (size_t -> int)
		disablewarnings "4204" -- nonstandard extension used: non-constant aggregate initializer
		disablewarnings "4057" -- function differs in indirection slightly
		disablewarnings "4131" -- uses old-style declarator -- libwebsockets includes old netbsd wrappers for some unix functions.
		disablewarnings "4706" -- assignment within conditional expression

	filter "toolset:*clang* or *gcc*"
		disablewarnings "incompatible-pointer-types"
		disablewarnings "implicit-fallthrough" -- this statement may fall through (expects [[fallthrough]] or compiler equivalent)
		disablewarnings "unused-but-set-parameter"
		disablewarnings "unused-variable"

local m = {}

function m.include_libwebsockets()
	config_websockets()
end

function m.link()
	filter {KIND_APP}
		links "mbedtls"
		links "libwebsockets"
	filter {"platforms:*win*", KIND_APP}
		links "ws2_32"
end

return m