/* ===========================================================================
 *
 * Copyright (c) 2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
http_status_codes.h

http status codes as enums for ease of use
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// extensions should have the extension name after: http_status_<extension_name>_<code_name> see websockets for example.

// http status codes as enums, copied from: https://developer.mozilla.org/en-US/docs/Web/HTTP/Status
typedef enum http_status_code_e {
    http_status_no_code = 0,

    // Information responses

    http_status_continue = 100,
    http_status_switching_protocol = 101,
    http_status_processing = 102,
    http_status_early_hints = 103,

    // Successful responses

    http_status_ok = 200,
    http_status_created = 201,
    http_status_accepted = 202,
    http_status_non_authoritative_information = 203,
    http_status_no_content = 204,
    http_status_reset_content = 205,
    http_status_partial_content = 206,
    http_status_multi_status = 207,
    http_status_already_reported = 208,
    http_status_IM_used = 226,

    // Redirection messages

    http_status_multiple_choice = 300,
    http_status_moved_permanently = 301,
    http_status_found = 302,
    http_status_see_other = 303,
    http_status_not_modified = 304,
    http_status_use_proxy = 305,
    // 306 is marked as unused
    http_status_temporary_redirect = 307,
    http_status_permanent_redirect = 308,

    // Client error responses

    http_status_bad_request = 400,
    http_status_unauthorized = 401,
    http_status_payment_required = 402,
    http_status_forbidden = 403,
    http_status_not_found = 404,
    http_status_method_not_allowed = 405,
    http_status_not_acceptable = 406,
    http_status_proxy_authentication_required = 407,
    http_status_request_timeout = 408,
    http_status_conflict = 409,
    http_status_gone = 410,
    http_status_length_required = 411,
    http_status_precondition_failed = 412,
    http_status_payload_too_large = 413,
    http_status_URI_too_long = 414,
    http_status_unsupported_media_type = 415,
    http_status_range_not_satisfiable = 416,
    http_status_expectation_failed = 417,
    http_status_im_a_teapot = 418,
    http_status_misdirected_request = 421,
    http_status_unprocessable_entity = 422,
    http_status_locked = 423,
    http_status_failed_dependency = 424,
    http_status_too_early = 425,
    http_status_upgrade_required = 426,
    http_status_precondition_required = 428,
    http_status_too_many_requests = 429,
    http_status_request_header_fields_too_large = 431,
    http_status_unavailable_for_legal_reasons = 451,

    // Server error responses

    http_status_internal_server_error = 500,
    http_status_not_implemented = 501,
    http_status_bad_gateway = 502,
    http_status_service_unavailable = 503,
    http_status_gateway_timeout = 504,
    http_status_http_version_not_supported = 505,
    http_status_variant_also_negotiates = 506,
    http_status_insufficient_storage = 507,
    http_status_loop_detected = 508,
    http_status_not_extended = 510,
    http_status_network_authentication_required = 511,

    // Websocket extension
    // https://tools.ietf.org/html/rfc6455#page-65

    http_status_websocket_normal_closure = 1000,
    http_status_websocket_going_away = 1001,
    http_status_websocket_protocol_error = 1002,
    http_status_websocket_unsupported_data = 1003,
    http_status_websocket_no_status_received = 1005,
    http_status_websocket_abnormal_closure = 1006,
    http_status_websocket_invalid_frame_payload_data = 1007,
    http_status_websocket_policy_violation = 1008,
    http_status_websocket_message_too_big = 1009,
    http_status_websocket_mandatory_extension = 1010,
    http_status_websocket_internal_server_error = 1011,
    http_status_websocket_tls_handshake = 1015,
} http_status_code_e;

#ifdef __cplusplus
}
#endif