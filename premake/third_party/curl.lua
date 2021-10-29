-------------------------------------------------------------------------------
-- curl.lua
-- Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
-- Builds curl for use in m5
-------------------------------------------------------------------------------

third_party_project "m5-curl"
	kind "staticlib"
		
	defines {
		"BUILDING_LIBCURL",
		"HAVE_ZLIB_H",
		"HAVE_CONFIG_H",
		"HTTP_ONLY",
		"HAVE_ASSERT_H",
		"HAVE_ERRNO_H",
		"HAVE_FCNTL_H",
		"HAVE_INTTYPES_H",
		"HAVE_IO_H",
		"HAVE_SIGNAL_H",
		"HAVE_STDBOOL_H",
		"HAVE_STDLIB_H",
		"HAVE_SYS_STAT_H",
		"HAVE_SYS_TYPES_H",
		"HAVE_TIME_H",
		"HAVE_SIG_ATOMIC_T",
		"STDC_HEADERS",
		"HAVE_BOOL_T",
		"HAVE_FTRUNCATE",
		"HAVE_GETADDRINFO", 
		"HAVE_GETADDRINFO_THREADSAFE",
		"HAVE_GETPEERNAME",
		"HAVE_GETSOCKNAME",
		"HAVE_GETHOSTBYADDR",
		"HAVE_GETSERVBYNAME",
		"HAVE_GETPROTOBYNAME",
		"HAVE_INET_ADDR",
		"HAVE_IOCTLSOCKET",
		"HAVE_PERROR",
		"HAVE_RAND_SCREEN",
		"HAVE_RAND_STATUS",
		"HAVE_CRYPTO_CLEANUP_ALL_EX_DATA",
		"HAVE_SELECT",
		"HAVE_SETLOCALE",
		"HAVE_SETMODE",
		"HAVE_SETVBUF",
		"HAVE_SOCKET",
		"HAVE_STRDUP",
		"HAVE_STRFTIME",
		"HAVE_STRICMP",
		"HAVE_STRSTR",
		"HAVE_STRTOLL",
		"HAVE_UTIME",
		"HAVE_LIBZ",
		"GETNAMEINFO_QUAL_ARG1=const",
		"GETNAMEINFO_TYPE_ARG1=struct sockaddr *",
		"GETNAMEINFO_TYPE_ARG2=socklen_t",
		"GETNAMEINFO_TYPE_ARG46=DWORD",
		"GETNAMEINFO_TYPE_ARG7=int",
		"HAVE_RECV",
		"RECV_TYPE_ARG4=int",
		"RECV_TYPE_ARG4=int",
		"HAVE_RECVFROM",
		"RECVFROM_TYPE_ARG2=char",
		"RECVFROM_TYPE_ARG3=int",
		"RECVFROM_TYPE_ARG4=int",
		"RECVFROM_TYPE_ARG5=struct sockaddr",
		"RECVFROM_TYPE_ARG6=int",
		"RECVFROM_TYPE_RETV=int",
		"HAVE_SEND",
		"SEND_QUAL_ARG2=const",
		"SEND_TYPE_ARG4=int",
		"RETSIGTYPE=void",
		"SIZEOF_INT=4",
		"SIZEOF_LONG_DOUBLE=16",
		"SIZEOF_SHORT=2",
		"SIZEOF_LONG=4",
		"SIZEOF_CURL_OFF_T=8",
		"HAVE_VARIADIC_MACROS_C99",
		"HAVE_LONGLONG",
		"HAVE_STRUCT_TIMEVAL",
		"HAVE_SOCKADDR_IN6_SIN6_SCOPE_ID",
		"USE_MBEDTLS",
		"USE_SB_SOCKET",
	}

filter {"platforms:not leia","platforms:not vader"}
	defines {
		"HAVE_GETHOSTNAME",
		"RECV_TYPE_ARG2=char *",	
		"RECV_TYPE_ARG3=int",
		"RECV_TYPE_RETV=int",
		"SEND_TYPE_ARG2=char *",
		"SEND_TYPE_ARG3=int",
		"SEND_TYPE_RETV=int"
	}

filter "platforms:*64"
	defines {
		"SIZEOF_TIME_T=8",
		"SIZEOF_SIZE_T=8"
	}

filter "platforms:*32"
	defines{
		"SIZEOF_TIME_T=4",
		"SIZEOF_SIZE_T=4"
	}

filter "platforms:*win*"
	defines {
		"in_addr_t=unsigned long",
		"HAVE_WINDOWS_H",
		"HAVE_WINSOCK2_H",
		"HAVE_PROCESS_H",
		"HAVE_STRUCT_POLLFD",
		"HAVE_IOCTLSOCKET_FIONBIO",
		"_CRT_SECURE_NO_DEPRECATE",
		"_CRT_NONSTDC_NO_DEPRECATE",
		"USE_WIN32_LARGE_FILES",
		"USE_THREADS_WIN32",
		"USE_WIN32_CRYPTO",
		"HAVE_FREEADDRINFO",
		"HAVE_GETNAMEINFO",
		"HAVE_WS2TCPIP_H",
		"HAVE_CLOSESOCKET",
		"RECV_TYPE_ARG1=SOCKET",
		"RECVFROM_TYPE_ARG1=SOCKET",
		"SEND_TYPE_ARG1=SOCKET",
		"ssize_t=__int64",
		'OS="x86_64-pc-win32"',
		'PACKAGE="curl"'
	}
filter "platforms:vader or leia"
	defines {
		"NEED_MEMORY_H",
		"HAVE_UNISTD_H",
		"HAVE_SYS_TIME_H",
		"HAVE_SYS_SOCKET_H",
		"HAVE_STRUCT_TIMEVAL",
		"HAVE_MSG_NOSIGNAL",
		"HAVE_RECV",
		"RECV_TYPE_ARG1=int",
		"RECV_TYPE_ARG2=void *",
		"RECV_TYPE_ARG3=size_t",
		"RECV_TYPE_ARG4=int",
		"RECV_TYPE_RETV=ssize_t",
		"SEND_TYPE_ARG1=int",
		"SEND_TYPE_ARG2=void *",
		"SEND_TYPE_ARG3=size_t",
		"SEND_TYPE_ARG4=int",
		"SEND_TYPE_RETV=ssize_t",
		'OS="ps"',
		"HAVE_NETDB_H",
		"SIZEOF_TIME_T=8",
		"SIZEOF_SIZE_T=8"
	}
filter "action:gmake*"
	defines {
		"HAVE_NETDB_H",
		"HAVE_UNISTD_H",
		"HAVE_SYS_SOCKET_H",
		"HAVE_STRTOK_R",
		"HAVE_FCNTL_O_NONBLOCK",
		"RECV_TYPE_ARG1=int",
		"RECVFROM_TYPE_ARG1=int",
		"SEND_TYPE_ARG1=int",
		'OS="x86_64-pc-deb"',
		'PACKAGE="curl"'
	}

filter "platforms:*brcm* or *rpi* or *mtk*" 
	disablewarnings "type-limits"

filter {}

includedirs "extern/curl" -- for our empty curl_config.
includedirs "extern/curl/curl/lib"
includedirs "extern/curl/curl/include"
includedirs "extern/zlib"
includedirs "extern/mbedtls/mbedtls/include"

filter "options:curl-http2"
	defines {
		"USE_NGHTTP2",
		"NGHTTP2_STATICLIB=1"
	}
	includedirs "extern/nghttp2-1.43.0/lib/includes"
filter {}

files {
	"curl/curl_config.h",
	"curl/curl/include/curl/**.h",
	"curl/curl/lib/**.h", 
	"curl/curl/lib/**.c"
}

removefiles "curl/curl/lib/config-*.h"

filter {"action:vs20*", "platforms:*win*"}
	defines "HAVE_WINDOWS_H"
	defines "HAVE_WINSOCK2_H"		

filter "action:vs20*"
	linkoptions "/IGNORE:4221" -- This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library
	disablewarnings "4101" -- unreferenced local variable

filter_console {}
	buildoptions "-include unistd_vader.h"

local m = {}

function m.link()
	filter {KIND_APP}
		links "m5-curl"
		links "m5-zlib"
		links "mbedtls"
	filter {"platforms:*win*", KIND_APP}
		links "ws2_32"
	filter "options:curl-http2"
		links "nghttp2_static"
end

return m
