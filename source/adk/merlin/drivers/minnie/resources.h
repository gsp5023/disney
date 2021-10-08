/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/// Path to the WASM application in the bundle
static const char bundle_app_wasm_path[] = "bin/app.wasm";

/// Path to the configuration file in the bundle. This file is optional and may not be in all bundles.
static const char bundle_config_path[] = "bin/.config";

/// Paths to the fallback error images in the bundle, listed in priority order.
/// These files are optional and may not be in all bundles.
/// If none of these are present in the bundle, it will use a local shared fallback image.
static const char * bundle_error_image_paths[] = {"resource/shared/fallback.jpg", "resource/shared/fallback.png"};

// Local path to the persona file
static const char default_persona_file_prod[] = "resource/shared/persona.json";
static const char default_persona_file_non_prod[] = "resource/shared/persona-dev.json";

// Local path to default error image when a bundle is not available
static const char default_error_image_path[] = "resource/shared/fallback.png";

// Default error message
static const char default_fallback_error_message[] = "Something went wrong. Try again later.";
