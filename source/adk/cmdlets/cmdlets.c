/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#include "source/adk/cmdlets/cmdlets.h"

#include "source/adk/runtime/runtime.h"

#if _RESTRICTED && !(defined(_STB_NATIVE) || defined(_CONSOLE_NATIVE) || defined(_RPI))
int cmdlet_bif_extract_tool(const int argc, const char * const * const argv);
int cmdlet_json_deflate_tool(const int argc, const char * const * const argv);
#endif

int cmdlet_http_test(const int argc, const char * const * const argv);

// Offset argc count and argv so that the commandlet gets
// the number of remaining commandline arguments and doens't need to
// iterate over parameter list to find its commandlet start parameters
#define RUN_CMDLET(str)                                    \
    if (strcasecmp(argv[argnum], STRINGIZE(str)) == 0) {   \
        return cmdlet_##str(argc - argnum, argv + argnum); \
    }

/*
commandlet main entry point

Returns non-zero on error or no commandlet specified
*/
int cmdlet_run(const int argc, const char * const * const argv) {
    int error_result = -1;
    // Ge the index of the parameter immediately following the commandlet flag
    int argnum = findarg(CMDLET_FLAG, argc, argv);
    if ((argnum != -1) && ((argnum + 1) < argc)) {
        argnum++;
    } else // no commandlet specified
    {
        debug_write_line("No commandlet flag (%s) specified", CMDLET_FLAG);
        return error_result;
    }

    // List of commandlets that can be invoked
    // Must have a method named cmdlet_<commandlet name>_tool
#if _RESTRICTED && !(defined(_STB_NATIVE) || defined(_CONSOLE_NATIVE) || defined(_RPI))
    RUN_CMDLET(bif_extract_tool);
    RUN_CMDLET(json_deflate_tool);
#endif

    RUN_CMDLET(http_test);

    debug_write_line("No commandlet with name: %s", argv[argnum]);

    return error_result;
}
