-------------------------------------------------------------------------------
-- nghttp2.lua
-- Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
-------------------------------------------------------------------------------

newoption {
	trigger = "curl-http2",
	description = "Build curl with http2 support",
}

if _OPTIONS["curl-http2"] == nil then
	return
end

defines {
    "ADK_CURL_HTTP2"
}

third_party_project "nghttp2_static"
	kind "staticlib"
    includedirs "extern/nghttp2-1.43.0/lib/includes"
    files "nghttp2-1.43.0/lib/*.h"
	files "nghttp2-1.43.0/lib/*.c"
    defines {
        "NGHTTP2_STATICLIB=1"
    }

    filter "platforms:*win*"
	defines {
		"ssize_t=__int64",
	}
    filter "toolset:msc*"
    disablewarnings {
        "4702",                     -- unreachable code
    }

    filter "platforms:not *win*"
    defines {
        "HAVE_NETINET_IN_H=1"
    }

    filter "platforms:*deb* or *rpi*"
    defines {
        "HAVE_ARPA_INET_H=1"
    }
    filter "toolset:*gcc*"
        disablewarnings {
            "strict-aliasing"
        }
    filter{}

local m = {}
function m.link()
	filter {KIND_APP}
		links "nghttp2_static"
end
return m