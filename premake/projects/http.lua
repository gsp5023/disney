-------------------------------------------------------------------------------
-- http.lua
-- Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
--------------------------------------------------------------------------------

project "http"
	group "adk"
	kind "staticlib"
	includedirs "extern/curl/curl/include"
	files { "**.h", "**.c" }
	MODULES.include_libwebsockets()

	filter { "platforms:not vader", "platforms:not leia" }
		if player_is_any("nve-internal") then
			defines "_ADK_NVE_CURL_SHARING=1"
		end

	-- TODO(M5-3317): remove curl-sharing for BME MIPS devices
	if player_is_any("nve-shared") then
		filter { "platforms:*brcm_bme*" }
			defines "_ADK_NVE_CURL_SHARING=1"
	end

	filter {}
