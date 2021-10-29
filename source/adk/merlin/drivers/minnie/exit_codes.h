/* ===========================================================================
 *
 * Copyright (c) 2020-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/// ADK error codes returned at exit
typedef enum merlin_exit_code_e {
    merlin_exit_code_success,
    merlin_exit_code_sb_preinit_failure,
    merlin_exit_code_cli_too_many_args,
    merlin_exit_code_cli_missing_parameter,
    merlin_exit_code_cli_redundant_action,
    merlin_exit_code_app_init_subsystems_failure,
    merlin_exit_code_persona_load_failure,
    merlin_exit_code_wasm_load_failure,
    merlin_exit_code_app_init_failure,
    merlin_exit_code_app_shutdown_failure,
    merlin_exit_code_extensions_failure,
    merlin_exit_code_invalid_path_as_argument,
    merlin_exit_code_unknown = 255
} merlin_exit_code_e;
