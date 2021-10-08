/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

#pragma once

/*
adk_http.h

steamboat http/s support via curl
*/

#include "source/adk/runtime/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
===============================================================================
These enums match their curl equivalents
===============================================================================
*/

/// Result status of an operation on a session handle
FFI_EXPORT FFI_NAME(adk_curl_result_e)
    FFI_TYPE_MODULE(http)
        FFI_ENUM_BITFLAGS
    FFI_ENUM_CLEAN_NAMES
    FFI_ENUM_CAPITALIZE_NAMES
    typedef enum adk_curl_result_e {
        adk_curl_result_busy = -1, // this is NOT a libcurl result, added to support ADK wrapper
        adk_curl_result_ok = 0,
        adk_curl_result_aborted_by_callback = 42,
        adk_curl_result_bad_calling_order = 44,
        adk_curl_result_bad_content_encoding = 61,
        adk_curl_result_bad_download_resume = 36,
        adk_curl_result_bad_function_argument = 43,
        adk_curl_result_bad_password_entered = 46,
        adk_curl_result_couldnt_connect = 7,
        adk_curl_result_couldnt_resolve_host = 6,
        adk_curl_result_couldnt_resolve_proxy = 5,
        adk_curl_result_failed_init = 2,
        adk_curl_result_filesize_exceeded = 63,
        adk_curl_result_file_couldnt_read_file = 37,
        adk_curl_result_function_not_found = 41,
        adk_curl_result_got_nothing = 52,
        adk_curl_result_http_post_error = 34,
        adk_curl_result_http_range_error = 33,
        adk_curl_result_http_returned_error = 22,
        adk_curl_result_interface_failed = 45,
        adk_curl_result_operation_timeouted = 28,
        adk_curl_result_out_of_memory = 27,
        adk_curl_result_partial_file = 18,
        adk_curl_result_read_error = 26,
        adk_curl_result_recv_error = 56,
        adk_curl_result_send_error = 55,
        adk_curl_result_send_fail_rewind = 65,
        adk_curl_result_share_in_use = 57,
        adk_curl_result_peer_failed_validation = 60,
        adk_curl_result_ssl_certproblem = 58,
        adk_curl_result_ssl_cipher = 59,
        adk_curl_result_ssl_connect_error = 35,
        adk_curl_result_ssl_engine_initfailed = 66,
        adk_curl_result_ssl_engine_notfound = 53,
        adk_curl_result_ssl_engine_setfailed = 54,
        adk_curl_result_ssl_peer_certificate = 51,
        adk_curl_result_too_many_redirects = 47,
        adk_curl_result_unsupported_protocol = 1,
        adk_curl_result_url_malformat = 3,
        adk_curl_result_url_malformat_user = 4,
        adk_curl_result_write_error = 23
    } adk_curl_result_e;

/// HTTP session handle options
FFI_EXPORT FFI_NAME(adk_curl_option_e)
    FFI_TYPE_MODULE(http)
        FFI_ENUM_BITFLAGS
    FFI_ENUM_CLEAN_NAMES
    FFI_ENUM_CAPITALIZE_NAMES
    typedef enum adk_curl_option_e {
        adk_curl_opt_auto_referer = 58,
        adk_curl_opt_buffer_size = 98,
        adk_curl_opt_cainfo = 10065,
        adk_curl_opt_capath = 10097,
        adk_curl_opt_connect_timeout = 78,
        adk_curl_opt_cookie = 10022,
        adk_curl_opt_cookie_file = 10031,
        adk_curl_opt_cookie_jar = 10082,
        adk_curl_opt_cookie_session = 96,
        adk_curl_opt_crlf = 27,
        adk_curl_opt_custom_request = 10036,
        adk_curl_opt_dns_cache_timeout = 92,
        adk_curl_opt_dns_use_global_cache = 91,
        adk_curl_opt_accept_encoding = 10102,
        adk_curl_opt_error_buffer = 10010,
        adk_curl_opt_fail_on_error = 45,
        adk_curl_opt_file_time = 69,
        adk_curl_opt_follow_location = 52,
        adk_curl_opt_forbid_reuse = 75,
        adk_curl_opt_fresh_connect = 74,
        adk_curl_opt_http_200_aliases = 10104,
        adk_curl_opt_http_auth = 107,
        adk_curl_opt_http_get = 80,
        adk_curl_opt_http_header = 10023,
        adk_curl_opt_http_post = 10024,
        adk_curl_opt_http_proxy_tunnel = 61,
        adk_curl_opt_http_version = 84,
        adk_curl_opt_in_file_size = 14,
        adk_curl_opt_interface = 10062,
        adk_curl_opt_ip_resolve = 113,
        adk_curl_opt_low_speed_limit = 19,
        adk_curl_opt_low_speed_time = 20,
        adk_curl_opt_max_connects = 71,
        adk_curl_opt_max_file_size = 114,
        adk_curl_opt_max_redirs = 68,
        adk_curl_opt_nobody = 44,
        adk_curl_opt_no_progress = 43,
        adk_curl_opt_no_signal = 99,
        adk_curl_opt_port = 3,
        adk_curl_opt_post = 47,
        adk_curl_opt_post_fields = 10015,
        adk_curl_opt_post_field_size = 60,
        adk_curl_opt_post_quote = 10039,
        adk_curl_opt_pre_quote = 10093,
        adk_curl_opt_private = 10103,
        adk_curl_opt_proxy = 10004,
        adk_curl_opt_proxy_auth = 111,
        adk_curl_opt_proxy_port = 59,
        adk_curl_opt_proxy_type = 101,
        adk_curl_opt_proxy_user_pwd = 10006,
        adk_curl_opt_put = 54,
        adk_curl_opt_quote = 10028,
        adk_curl_opt_random_file = 10076,
        adk_curl_opt_range = 10007,
        adk_curl_opt_read_data = 10009,
        adk_curl_opt_read_function = 20012,
        adk_curl_opt_referer = 10016,
        adk_curl_opt_resume_from = 21,
        adk_curl_opt_share = 10100,
        adk_curl_opt_source_host = 10122,
        adk_curl_opt_source_path = 10124,
        adk_curl_opt_source_port = 125,
        adk_curl_opt_source_post_quote = 10128,
        adk_curl_opt_source_pre_quote = 10127,
        adk_curl_opt_source_quote = 10133,
        adk_curl_opt_source_url = 10132,
        adk_curl_opt_source_userpwd = 10123,
        adk_curl_opt_ssl_cert = 10025,
        adk_curl_opt_ssl_cert_passwd = 10026,
        adk_curl_opt_ssl_cert_type = 10086,
        adk_curl_opt_ssl_engine = 10089,
        adk_curl_opt_ssl_engine_default = 90,
        adk_curl_opt_ssl_key = 10087,
        adk_curl_opt_ssl_keypasswd = 10026,
        adk_curl_opt_ssl_keytype = 10088,
        adk_curl_opt_ssl_version = 32,
        adk_curl_opt_ssl_cipher_list = 10083,
        adk_curl_opt_ssl_verify_host = 81,
        adk_curl_opt_ssl_verify_peer = 64,
        adk_curl_opt_tcp_nodelay = 121,
        adk_curl_opt_time_condition = 33,
        adk_curl_opt_timeout = 13,
        adk_curl_opt_time_value = 34,
        adk_curl_opt_transfer_text = 53,
        adk_curl_opt_unrestricted_auth = 105,
        adk_curl_opt_upload = 46,
        adk_curl_opt_url = 10002,
        adk_curl_opt_user_agent = 10018,
        adk_curl_opt_user_pwd = 10005,
        adk_curl_opt_verbose = 41
    } adk_curl_option_e;

/// HTTP session handle information
FFI_EXPORT FFI_NAME(adk_curl_info_e)
    FFI_TYPE_MODULE(http)
        FFI_ENUM_BITFLAGS
    FFI_ENUM_CLEAN_NAMES
    FFI_ENUM_CAPITALIZE_NAMES
    typedef enum adk_curl_info_e {
        adk_curl_info_connect_time = 0x300005,
        adk_curl_info_content_length_download = 0x30000f,
        adk_curl_info_content_length_upload = 0x300010,
        adk_curl_info_content_type = 0x100012,
        adk_curl_info_effective_url = 0x100001,
        adk_curl_info_filetime = 0x20000e,
        adk_curl_info_header_size = 0x20000b,
        adk_curl_info_httpauth_avail = 0x200017,
        adk_curl_info_http_connect_code = 0x200016,
        adk_curl_info_lastone = 0x1c,
        adk_curl_info_name_lookup_time = 0x300004,
        adk_curl_info_none = 0x0,
        adk_curl_info_num_connects = 0x20001a,
        adk_curl_info_os_errno = 0x200019,
        adk_curl_info_pretransfer_time = 0x300006,
        adk_curl_info_private = 0x100015,
        adk_curl_info_proxy_auth_avail = 0x200018,
        adk_curl_info_redirect_count = 0x200014,
        adk_curl_info_redirect_time = 0x300013,
        adk_curl_info_request_size = 0x20000c,
        adk_curl_info_response_code = 0x200002,
        adk_curl_info_size_download = 0x300008,
        adk_curl_info_size_upload = 0x300007,
        adk_curl_info_speed_download = 0x300009,
        adk_curl_info_speed_upload = 0x30000a,
        adk_curl_info_ssl_engines = 0x40001b,
        adk_curl_info_ssl_verify_result = 0x20000d,
        adk_curl_info_start_transfer_time = 0x300011,
        adk_curl_info_total_time = 0x300003
    } adk_curl_info_e;

/// Session handle buffering mode
FFI_EXPORT FFI_NAME(adk_curl_handle_buffer_mode_e)
    FFI_TYPE_MODULE(http)
        FFI_ENUM_BITFLAGS
    FFI_ENUM_TRIM_START_NAMES(adk_curl_handle_)
        FFI_ENUM_CAPITALIZE_NAMES
    typedef enum adk_curl_handle_buffer_mode_e {
        adk_curl_handle_buffer_off = 0x0,
        adk_curl_handle_buffer_http_body = 0x1,
        adk_curl_handle_buffer_http_header = 0x2,
    } adk_curl_handle_buffer_mode_e;

/// HTTP session handle
///
/// NOTE: HTTP api is _not_ currently thread-safe, but it is non-blocking
typedef struct adk_curl_handle_t adk_curl_handle_t;

/// Linked-list of strings
typedef struct adk_curl_slist_t adk_curl_slist_t;

/// Callbacks for notification during async operations (see `adk_curl_async_perform`)
typedef struct adk_curl_callbacks_t {
    /// `on_http_header_recv` receives a block of one or more complete response header lines.  Lines are terminated by CR LF (not null terminated).
    /// The first line will be the status line and the last line will be empty.  The header name and value are ASCII although the callback should
    /// tolerate obsolete values containing ISO-8859-1 characters (0x80-0xff).
    bool (*on_http_header_recv)(
        adk_curl_handle_t * const handle,
        const const_mem_region_t bytes,
        const struct adk_curl_callbacks_t * const callbacks);

    /// `on_http_body_recv` passes the next sequential chunk of the response body, which must be interpreted according to the content type.
    bool (*on_http_recv)(
        adk_curl_handle_t * const handle,
        const const_mem_region_t bytes,
        const struct adk_curl_callbacks_t * const callbacks);

    /// `on_complete` is called after the response has been received, or the request has failed.
    /// It provides the result code (not the HTTP response status code).
    void (*on_complete)(
        adk_curl_handle_t * const handle,
        const adk_curl_result_e result,
        const struct adk_curl_callbacks_t * const callbacks);

    void * user[3];
} adk_curl_callbacks_t;

/// Choice between normal and full initialization
typedef enum adk_curl_http_init_mode_e {
    adk_curl_http_init_minimal = 0,
    adk_curl_http_init_normal = 1
} adk_curl_http_init_mode_e;

/// Initializes the HTTP system
bool adk_curl_api_init(
    const mem_region_t region,
    const mem_region_t fragments_region,
    const uint32_t fragment_size,
    const system_guard_page_mode_e guard_page_mode,
    adk_curl_http_init_mode_e init_mode);

/// Shuts down the HTTP system
void adk_curl_api_shutdown();

/// Shuts down all outstanding http handles (this should be called before adk_curl_api_shutdown())
void adk_curl_api_shutdown_all_handles();

/// dumps internal heap usage
void adk_curl_dump_heap_usage();

/// Returns true if any IO was processed
bool adk_curl_run_callbacks();

/// Creates new HTTP session handle
FFI_EXPORT
FFI_PTR_NATIVE adk_curl_handle_t * adk_curl_open_handle();

/// Closes HTTP session handle
FFI_EXPORT FFI_NAME(adk_curl_close_handle) void adk_curl_close_handle(FFI_PTR_NATIVE adk_curl_handle_t * const handle);

/// Returns the buffering mode of the session handle
FFI_EXPORT adk_curl_handle_buffer_mode_e adk_curl_get_buffering_mode(FFI_PTR_NATIVE adk_curl_handle_t * const handle);

/// Configures the buffering mode of the session handle
FFI_EXPORT FFI_NAME(adk_curl_set_buffering_mode) void adk_curl_set_buffering_mode(
    FFI_PTR_NATIVE adk_curl_handle_t * const handle,
    const adk_curl_handle_buffer_mode_e buffer_mode);

/// Returns the HTTP body of the session handle
const_mem_region_t adk_curl_get_http_body(adk_curl_handle_t * const handle);

/// Returns the HTTP header of the session handle
const_mem_region_t adk_curl_get_http_header(adk_curl_handle_t * const handle);

/// Appends the string `sz` to the slist `list`
FFI_EXPORT
FFI_PTR_NATIVE adk_curl_slist_t * adk_curl_slist_append(
    FFI_PTR_NATIVE FFI_CAN_BE_NULL adk_curl_slist_t * list,
    FFI_PTR_WASM const char * const sz);

/// Releases `list` and all elements in the list
FFI_EXPORT void adk_curl_slist_free_all(FFI_PTR_NATIVE adk_curl_slist_t * const list);

/// Sets an option `opt` on the session `handle` to the value `arg`
FFI_EXPORT FFI_NAME(adk_curl_set_opt_i32) void adk_curl_set_opt_long(
    FFI_PTR_NATIVE adk_curl_handle_t * const handle,
    const adk_curl_option_e opt,
    FFI_TYPE_OVERRIDE(int32_t) const long arg);

/// Sets an option `opt` on the session `handle` to the value `arg`
FFI_EXPORT FFI_UNSAFE void adk_curl_set_opt_ptr(
    FFI_PTR_NATIVE adk_curl_handle_t * const handle,
    const adk_curl_option_e opt,
    FFI_PTR_WASM FFI_SLICE const void * const arg);

/// Extracts session handle `info` into `out`
adk_curl_result_e adk_curl_get_info_long(adk_curl_handle_t * const handle, const adk_curl_info_e info, long * const out);

/// Performs an async operation on the session handle, notifying progress via `callbacks`
void adk_curl_async_perform(adk_curl_handle_t * const handle, const adk_curl_callbacks_t callbacks);

/// Returns the resulting state of the session handle
adk_curl_result_e adk_curl_async_get_result(adk_curl_handle_t * const handle);

void adk_curl_set_json_deflate_callback(
    adk_curl_handle_t * const handle,
    void (*json_deflate_callback)(void * const args),
    void * args);

#ifdef __cplusplus
}
#endif
