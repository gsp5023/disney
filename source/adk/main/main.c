/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
main.c

Entrypoint for all ADK applications.
*/

#include "source/adk/runtime/runtime.h"

/*
===============================================================================
unified_main

Unified application entry point for D+/E+ and other applications built with the m5 ADK.
Platform specific main() is implemented for supported platforms where they decompose
the command line into the standard C argc, argv equivalents and pass control onto the
unified main entry point.
===============================================================================
*/

#if defined(_VADER) || defined(_LEIA)
int adk_main(const int argc, const char * const * const argv);

int main(int argc, char ** argv) {
    return adk_main(argc, PEDANTIC_CAST(const char * const * const) argv);
}

#elif defined(__linux__)
int thunk_main(const int argc, const char * const * const argv);

int main(int argc, char ** argv) {
    return thunk_main(argc, PEDANTIC_CAST(const char * const * const) argv);
}

#elif defined(_WIN32)

/*
===============================================================================
WinMain

Windows CRT entrypoint.
===============================================================================
*/

#define NEED_SPLIT_COMMAND_LINE

/*
===============================================================================
split_command_line

For platforms that provide the engine command line in a single string, decompose
it into an array of char * and fill in the number.
===============================================================================
*/

/* defined NEED_SPLIT_COMMAND_LINE if your main needs this otherwise it will not be implemented */
static int split_command_line(char * cmdline, char *** argv);

int thunk_main(const int argc, const char * const * const argv);

int WinMain(void * h_instance, void * h_prev_instance, char * cmdline, int ncmdshow) {
    char ** argv;
    int argc = split_command_line(cmdline, &argv);

    h_instance;
    h_prev_instance;
    ncmdshow;

    return thunk_main(argc, argv);
}

#endif

#ifdef NEED_SPLIT_COMMAND_LINE

static char * skip_whitespace(char * sz) {
    while (*sz && (*sz <= ' ')) {
        ++sz;
    }
    return sz;
}

enum {
    max_cmd_arg_len = 256,
    max_cmd_args = 256 // TODO: align merlin/main.c:max_argv to common header
};

static int split_command_line(char * cmdline, char *** outargv) {
    int argc = 0;
    int quote = 0;
    int toklen = 0;

    static char argv[max_cmd_args][max_cmd_arg_len];
    static char * argv_pointers[max_cmd_args];
    /* null terminate */
    memset(argv, 0, sizeof(argv));
    memset(argv_pointers, 0, sizeof(argv_pointers));

    while (*cmdline) {
        if (argc == max_cmd_args) {
            break;
        }

        if (quote) {
            if (*cmdline == '"') {
                quote = 0;
                toklen = 0;
                ++argc;
                ++cmdline;
                continue;
            }
        } else if (toklen == 0) {
            cmdline = skip_whitespace(cmdline);
            if (!*cmdline) {
                break;
            }

            if (*cmdline == '"') {
                quote = 1;
                ++cmdline;
                continue;
            }
        } else if (*cmdline <= ' ') {
            /* whitespace starts new token */
            toklen = 0;
            ++argc;
            continue;
        }

        argv[argc][toklen++] = *(cmdline++);

        /* MAX_CMD_ARG_LEN-1 to keep null terminator. */
        if (toklen == max_cmd_arg_len - 1) {
            break;
        }
    }

    if ((argc < max_cmd_args) && toklen) {
        ++argc;
    }
    for (int i = 0; i < argc; ++i) {
        argv_pointers[i] = &argv[i][0];
    }
    *outargv = (char **)argv_pointers;
    return argc;
}
#endif
