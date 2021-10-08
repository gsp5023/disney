-------------------------------------------------------------------------------
-- link_adk.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

-- automatically link all the adk libs

local m = {}

function m.link(prj)

    filter {KIND_APP}
        links {
            "runtime",
            "paddleboat",
            "steamboat-cpp",
            "renderer",
            "cncbus",
            "extender",
            "log",
            "app_thunk",
            "interpreter",
            "json_deflate",
            "http",
            "imagelib",
            "cache",
            "canvas",
            "ffi",
            "metrics",
            "manifest",
            "bundle",
            "splash",
            "file",
            "reporting",
            "persona",
            "coredump",
            "m5-crypto",
        }

    filter {KIND_APP, "configurations:not *ship*"}
		links "cmdlets"
end

return m
