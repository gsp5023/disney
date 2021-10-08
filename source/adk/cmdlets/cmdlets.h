/* ===========================================================================
 *
 * Copyright (c) 2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*

cmdlet.h

Provides support for running a special command instead of the regular application.

Usage:

app --cmdlet secure_load_tool encrypt rust_app.wasm rust_app_encrypted.wasm

*/

/*
This flag indicates a commandlet is being requested to run
*/
static const char * const CMDLET_FLAG = "--cmdlet";

/*
=======================================
cmdlet_run

Should be called in place of normal program execution.
=======================================
*/

int cmdlet_run(const int argc, const char * const * const argv);

#ifdef __cplusplus
}
#endif
