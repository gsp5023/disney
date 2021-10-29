/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
 * This is generated code. It will be regenerated on app build. Do not modify manually.
 */

#include "source/adk/extender/generated/ffi.h"
static struct { const adk_native_functions_t * functions; } statics;
bool extensions_ffi_init(const adk_native_functions_t * const f) {
    statics.functions = f;
    const bool ffi_table_valid = (statics.functions->ffi_table_hash == extensions_ffi_table_hash);
    ASSERT(ffi_table_valid);
    return ffi_table_valid;
}
void assert_failed(const char *const message, const char *const filename, const char *const function, const int line) {  statics.functions->assert_failed(message, filename, function, line); }
void sb_vadebug_write_line(const char *const msg, va_list args) {  statics.functions->sb_vadebug_write_line(msg, args); }
void sb_unformatted_debug_write_line(const char *const msg) {  statics.functions->sb_unformatted_debug_write_line(msg); }
sb_file_t * sb_fopen(const sb_file_directory_e directory, const char *const path, const char *const mode) { return statics.functions->sb_fopen(directory, path, mode); }
_Bool sb_fclose(sb_file_t *const file) { return statics.functions->sb_fclose(file); }
_Bool sb_delete_file(const sb_file_directory_e directory, const char *const filename) { return statics.functions->sb_delete_file(directory, filename); }
sb_directory_delete_error_e sb_delete_directory(const sb_file_directory_e directory, const char *const subpath) { return statics.functions->sb_delete_directory(directory, subpath); }
_Bool sb_create_directory_path(const sb_file_directory_e directory, const char *const input_path) { return statics.functions->sb_create_directory_path(directory, input_path); }
sb_directory_t * sb_open_directory(const sb_file_directory_e mount_point, const char *const subpath) { return statics.functions->sb_open_directory(mount_point, subpath); }
void sb_close_directory(sb_directory_t *const directory) {  statics.functions->sb_close_directory(directory); }
sb_read_directory_result_t sb_read_directory(sb_directory_t *const directory) { return statics.functions->sb_read_directory(directory); }
const char * sb_get_directory_entry_name(const sb_directory_entry_t *const directory_entry) { return statics.functions->sb_get_directory_entry_name(directory_entry); }
sb_stat_result_t sb_stat(const sb_file_directory_e mount_point, const char *const subpath) { return statics.functions->sb_stat(mount_point, subpath); }
long sb_ftell(sb_file_t *const file) { return statics.functions->sb_ftell(file); }
_Bool sb_fseek(sb_file_t *const file, const long offset, const sb_seek_mode_e origin) { return statics.functions->sb_fseek(file, offset, origin); }
size_t sb_fread(void *const buffer, const size_t elem_size, const size_t elem_count, sb_file_t *const file) { return statics.functions->sb_fread(buffer, elem_size, elem_count, file); }
size_t sb_fwrite(const void *const buffer, const size_t elem_size, const size_t elem_count, sb_file_t *const file) { return statics.functions->sb_fwrite(buffer, elem_size, elem_count, file); }
_Bool sb_feof(sb_file_t *const file) { return statics.functions->sb_feof(file); }
_Bool sb_rename(const sb_file_directory_e mount_point, const char *const current_path, const char *const new_path) { return statics.functions->sb_rename(mount_point, current_path, new_path); }
bundle_t * bundle_open(const sb_file_directory_e directory, const char *const path) { return statics.functions->bundle_open(directory, path); }
_Bool bundle_close(bundle_t *const bundle) { return statics.functions->bundle_close(bundle); }
microseconds_t adk_read_microsecond_clock() { return statics.functions->adk_read_microsecond_clock(); }
milliseconds_t adk_read_millisecond_clock() { return statics.functions->adk_read_millisecond_clock(); }
sb_thread_id_t sb_get_main_thread_id() { return statics.functions->sb_get_main_thread_id(); }
sb_thread_id_t sb_get_current_thread_id() { return statics.functions->sb_get_current_thread_id(); }
sb_thread_id_t sb_create_thread(const char *name, const sb_thread_options_t options, int (*const thread_proc)(void *), void *const arg, const char *const tag) { return statics.functions->sb_create_thread(name, options, thread_proc, arg, tag); }
_Bool sb_set_thread_priority(const sb_thread_id_t id, const sb_thread_priority_e priority) { return statics.functions->sb_set_thread_priority(id, priority); }
void sb_join_thread(const sb_thread_id_t id) {  statics.functions->sb_join_thread(id); }
void sb_thread_sleep(const milliseconds_t time) {  statics.functions->sb_thread_sleep(time); }
sb_mutex_t * sb_create_mutex(const char *const tag) { return statics.functions->sb_create_mutex(tag); }
void sb_destroy_mutex(sb_mutex_t *const mutex, const char *const tag) {  statics.functions->sb_destroy_mutex(mutex, tag); }
void sb_lock_mutex(sb_mutex_t *const mutex) {  statics.functions->sb_lock_mutex(mutex); }
void sb_unlock_mutex(sb_mutex_t *const mutex) {  statics.functions->sb_unlock_mutex(mutex); }
sb_condition_variable_t * sb_create_condition_variable(const char *const tag) { return statics.functions->sb_create_condition_variable(tag); }
void sb_destroy_condition_variable(sb_condition_variable_t *const cnd, const char *const tag) {  statics.functions->sb_destroy_condition_variable(cnd, tag); }
void sb_condition_wake_one(sb_condition_variable_t *cnd) {  statics.functions->sb_condition_wake_one(cnd); }
void sb_condition_wake_all(sb_condition_variable_t *cnd) {  statics.functions->sb_condition_wake_all(cnd); }
_Bool sb_wait_condition(sb_condition_variable_t *cnd, sb_mutex_t *mutex, const milliseconds_t timeout) { return statics.functions->sb_wait_condition(cnd, mutex, timeout); }
int get_sys_page_size() { return statics.functions->get_sys_page_size(); }
void heap_init_with_region(heap_t *const heap, const mem_region_t region, const int alignment, const int block_header_size, const char *const name) {  statics.functions->heap_init_with_region(heap, region, alignment, block_header_size, name); }
void heap_destroy(heap_t *const heap, const char *const tag) {  statics.functions->heap_destroy(heap, tag); }
void * heap_unchecked_alloc(heap_t *const heap, const size_t size, const char *const tag) { return statics.functions->heap_unchecked_alloc(heap, size, tag); }
void * heap_unchecked_calloc(heap_t *const heap, size_t size, const char *const tag) { return statics.functions->heap_unchecked_calloc(heap, size, tag); }
void * heap_unchecked_realloc(heap_t *const heap, void *const ptr, const size_t size, const char *const tag) { return statics.functions->heap_unchecked_realloc(heap, ptr, size, tag); }
void heap_free(heap_t *const heap, void *const ptr, const char *const tag) {  statics.functions->heap_free(heap, ptr, tag); }
void heap_debug_print_leaks(const heap_t *const heap) {  statics.functions->heap_debug_print_leaks(heap); }
void * memory_pool_unchecked_alloc(memory_pool_t *const pool, const char *const tag) { return statics.functions->memory_pool_unchecked_alloc(pool, tag); }
void cncbus_init(cncbus_t *const bus, const mem_region_t region, const system_guard_page_mode_e guard_pages) {  statics.functions->cncbus_init(bus, region, guard_pages); }
void cncbus_destroy(cncbus_t *const bus) {  statics.functions->cncbus_destroy(bus); }
cncbus_dispatch_result_e cncbus_dispatch(cncbus_t *const bus, const cncbus_dispatch_mode_e mode) { return statics.functions->cncbus_dispatch(bus, mode); }
void cncbus_init_receiver(cncbus_receiver_t *const receiver, const struct cncbus_receiver_vtable_t *const vtable, const cncbus_address_t address) {  statics.functions->cncbus_init_receiver(receiver, vtable, address); }
void cncbus_connect(cncbus_t *const bus, cncbus_receiver_t *const receiver) {  statics.functions->cncbus_connect(bus, receiver); }
void cncbus_disconnect(cncbus_t *const bus, cncbus_receiver_t *const receiver) {  statics.functions->cncbus_disconnect(bus, receiver); }
cncbus_msg_t * cncbus_msg_begin_unchecked(cncbus_t *const bus, const uint32_t msg_type) { return statics.functions->cncbus_msg_begin_unchecked(bus, msg_type); }
_Bool cncbus_msg_reserve_unchecked(cncbus_msg_t *const msg, const int size) { return statics.functions->cncbus_msg_reserve_unchecked(msg, size); }
_Bool cncbus_msg_write_unchecked(cncbus_msg_t *const msg, const void *const src, const int size) { return statics.functions->cncbus_msg_write_unchecked(msg, src, size); }
_Bool cncbus_msg_set_size_unchecked(cncbus_msg_t *const msg, int size) { return statics.functions->cncbus_msg_set_size_unchecked(msg, size); }
void cncbus_msg_cancel(cncbus_msg_t *const msg) {  statics.functions->cncbus_msg_cancel(msg); }
int cncbus_msg_read(cncbus_msg_t *const msg, void *const dst, const int size) { return statics.functions->cncbus_msg_read(msg, dst, size); }
void cncbus_send_async(cncbus_msg_t *const msg, const cncbus_address_t source_address, const cncbus_address_t dest_address, const cncbus_address_t subnet_mask, cncbus_signal_t *const signal) {  statics.functions->cncbus_send_async(msg, source_address, dest_address, subnet_mask, signal); }
_Bool sb_preinit(const int argc, const char *const *const argv) { return statics.functions->sb_preinit(argc, argv); }
_Bool sb_init(struct adk_api_t *api, const int argc, const char *const *const argv, const system_guard_page_mode_e guard_page_mode) { return statics.functions->sb_init(api, argc, argv, guard_page_mode); }
void sb_shutdown() {  statics.functions->sb_shutdown(); }
void sb_halt(const char *const message) {  statics.functions->sb_halt(message); }
void sb_platform_dump_heap_usage() {  statics.functions->sb_platform_dump_heap_usage(); }
heap_metrics_t sb_platform_get_heap_metrics() { return statics.functions->sb_platform_get_heap_metrics(); }
mem_region_t sb_map_pages(const size_t size, const system_page_protect_e protect) { return statics.functions->sb_map_pages(size, protect); }
void sb_protect_pages(const mem_region_t pages, const system_page_protect_e protect) {  statics.functions->sb_protect_pages(pages, protect); }
void sb_unmap_pages(const mem_region_t pages) {  statics.functions->sb_unmap_pages(pages); }
void sb_assert_failed(const char *const message, const char *const filename, const char *const function, const int line) {  statics.functions->sb_assert_failed(message, filename, function, line); }
void sb_on_app_load_failure() {  statics.functions->sb_on_app_load_failure(); }
void sb_notify_app_status(const sb_app_notify_e notify) {  statics.functions->sb_notify_app_status(notify); }
void sb_get_system_metrics(struct adk_system_metrics_t *const out) {  statics.functions->sb_get_system_metrics(out); }
void sb_tick(const struct adk_event_t **const head, const struct adk_event_t **const tail) {  statics.functions->sb_tick(head, tail); }
const_mem_region_t sb_get_deeplink_buffer(const sb_deeplink_handle_t *const handle) { return statics.functions->sb_get_deeplink_buffer(handle); }
void sb_release_deeplink(sb_deeplink_handle_t *const handle) {  statics.functions->sb_release_deeplink(handle); }
nanoseconds_t sb_read_nanosecond_clock() { return statics.functions->sb_read_nanosecond_clock(); }
sb_time_since_epoch_t sb_get_time_since_epoch() { return statics.functions->sb_get_time_since_epoch(); }
void sb_seconds_since_epoch_to_localtime(const time_t seconds, struct tm *const _tm) {  statics.functions->sb_seconds_since_epoch_to_localtime(seconds, _tm); }
void sb_text_to_speech(const char *const text) {  statics.functions->sb_text_to_speech(text); }
sb_uuid_t sb_generate_uuid() { return statics.functions->sb_generate_uuid(); }
void sb_report_app_metrics(const char *const app_id, const char *const app_name, const char *const app_version) {  statics.functions->sb_report_app_metrics(app_id, app_name, app_version); }
sb_cpu_mem_status_t sb_get_cpu_mem_status() { return statics.functions->sb_get_cpu_mem_status(); }
void crypto_generate_hmac(const const_mem_region_t key, const const_mem_region_t input, uint8_t output[32]) {  statics.functions->crypto_generate_hmac(key, input, output); }
size_t crypto_encode_base64(const const_mem_region_t input, const mem_region_t output) { return statics.functions->crypto_encode_base64(input, output); }
_Bool adk_mount_bundle(bundle_t *const bundle) { return statics.functions->adk_mount_bundle(bundle); }
_Bool adk_unmount_bundle(bundle_t *const bundle) { return statics.functions->adk_unmount_bundle(bundle); }
sb_file_t * adk_fopen(const sb_file_directory_e directory, const char *const path, const char *const mode) { return statics.functions->adk_fopen(directory, path, mode); }
_Bool adk_fclose(sb_file_t *const file) { return statics.functions->adk_fclose(file); }
sb_stat_result_t adk_stat(const sb_file_directory_e mount_point, const char *const subpath) { return statics.functions->adk_stat(mount_point, subpath); }
size_t adk_fread(void *const buffer, const size_t elem_size, const size_t elem_count, sb_file_t *const file) { return statics.functions->adk_fread(buffer, elem_size, elem_count, file); }
void adk_http_init(const char *const ssl_certificate_path, const mem_region_t region, const enum adk_websocket_backend_e backend, const struct websocket_config_t config, const system_guard_page_mode_e guard_page_mode, const char *const tag) {  statics.functions->adk_http_init(ssl_certificate_path, region, backend, config, guard_page_mode, tag); }
void adk_http_shutdown(const char *const tag) {  statics.functions->adk_http_shutdown(tag); }
void adk_http_dump_heap_usage() {  statics.functions->adk_http_dump_heap_usage(); }
heap_metrics_t adk_http_get_heap_metrics() { return statics.functions->adk_http_get_heap_metrics(); }
_Bool adk_http_tick() { return statics.functions->adk_http_tick(); }
adk_http_header_list_t * adk_http_append_header_list(adk_http_header_list_t *list, const char *const name, const char *const value, const char *const tag) { return statics.functions->adk_http_append_header_list(list, name, value, tag); }
adk_websocket_handle_t * adk_websocket_create(const char *const url, const char *const supported_protocols, adk_http_header_list_t *const header_list, const adk_websocket_callbacks_t callbacks, const char *const tag) { return statics.functions->adk_websocket_create(url, supported_protocols, header_list, callbacks, tag); }
adk_websocket_handle_t * adk_websocket_create_with_ssl_ctx(const char *const url, const char *const supported_protocols, adk_http_header_list_t *const header_list, const adk_websocket_callbacks_t callbacks, mem_region_t ssl_ca, mem_region_t ssl_client, const char *const tag) { return statics.functions->adk_websocket_create_with_ssl_ctx(url, supported_protocols, header_list, callbacks, ssl_ca, ssl_client, tag); }
void adk_websocket_close(adk_websocket_handle_t *const ws_handle, const char *const tag) {  statics.functions->adk_websocket_close(ws_handle, tag); }
adk_websocket_status_e adk_websocket_get_status(adk_websocket_handle_t *const ws_handle) { return statics.functions->adk_websocket_get_status(ws_handle); }
adk_websocket_status_e adk_websocket_send(adk_websocket_handle_t *const ws_handle, const const_mem_region_t message, const adk_websocket_message_type_e message_type, const adk_websocket_callbacks_t write_status_callback) { return statics.functions->adk_websocket_send(ws_handle, message, message_type, write_status_callback); }
adk_websocket_message_type_e adk_websocket_begin_read(adk_websocket_handle_t *const ws_handle, const_mem_region_t *const message) { return statics.functions->adk_websocket_begin_read(ws_handle, message); }
void adk_websocket_end_read(adk_websocket_handle_t *const ws_handle, const char *const tag) {  statics.functions->adk_websocket_end_read(ws_handle, tag); }
void wasm_sig(wasm_sig_mask mask, char *const out) {  statics.functions->wasm_sig(mask, out); }
void * wasm_translate_ptr_wasm_to_native(wasm_ptr_t addr) { return statics.functions->wasm_translate_ptr_wasm_to_native(addr); }
#ifdef _WASM3
void wasm3_export_native_function(IM3Module module, const char *const name, const wasm_sig_mask signature, const M3RawCall func_ptr) {  statics.functions->wasm3_export_native_function(module, name, signature, func_ptr); }
#endif // _WASM3
#ifdef _WAMR
void wamr_export_native_function(wasm_exec_env_t env, const char *const name, const wasm_sig_mask signature, const uintptr_t func_ptr) {  statics.functions->wamr_export_native_function(env, name, signature, func_ptr); }
#endif // _WAMR
const char * log_get_level_short_name(const log_level_e level) { return statics.functions->log_get_level_short_name(level); }
log_level_e log_get_min_level() { return statics.functions->log_get_min_level(); }
void log_message(const char *const file, const int line, const char *const func, const log_level_t level, const uint32_t fourcc_tag, const char *const msg, va_list args) {  statics.functions->log_message(file, line, func, level, fourcc_tag, msg, args); }
void log_init(cncbus_t *const bus, const cncbus_address_t address, const cncbus_address_t subnet_mask, adk_reporting_instance_t *const reporting_instance) {  statics.functions->log_init(bus, address, subnet_mask, reporting_instance); }
void log_shutdown() {  statics.functions->log_shutdown(); }
void log_receiver_init(cncbus_t *const bus, const cncbus_address_t address) {  statics.functions->log_receiver_init(bus, address); }
void log_receiver_shutdown(cncbus_t *const bus) {  statics.functions->log_receiver_shutdown(bus); }
_Bool sb_enumerate_display_modes(const int32_t display_index, const int32_t display_mode_index, sb_enumerate_display_modes_result_t *const out_results) { return statics.functions->sb_enumerate_display_modes(display_index, display_mode_index, out_results); }
sb_window_t * sb_init_main_display(const int display_index, const int display_mode_index, const char *const title) { return statics.functions->sb_init_main_display(display_index, display_mode_index, title); }
_Bool sb_set_main_display_refresh_rate(const int32_t hz) { return statics.functions->sb_set_main_display_refresh_rate(hz); }
void sb_get_window_client_area(sb_window_t *const window, int *const out_width, int *const out_height) {  statics.functions->sb_get_window_client_area(window, out_width, out_height); }
void sb_destroy_main_window() {  statics.functions->sb_destroy_main_window(); }
void adk_get_system_metrics(adk_system_metrics_t *const out) {  statics.functions->adk_get_system_metrics(out); }
rb_cmd_buf_t * render_get_cmd_buf(render_device_t *const device, const render_wait_mode_e wait_mode) { return statics.functions->render_get_cmd_buf(device, wait_mode); }
void adk_app_metrics_init() {  statics.functions->adk_app_metrics_init(); }
void adk_app_metrics_shutdown() {  statics.functions->adk_app_metrics_shutdown(); }
void adk_app_metrics_report(const char *const app_id, const char *const app_name, const char *const app_version) {  statics.functions->adk_app_metrics_report(app_id, app_name, app_version); }
adk_app_metrics_result_e adk_app_metrics_get(adk_app_metrics_t *app_info) { return statics.functions->adk_app_metrics_get(app_info); }
void adk_lock_events() {  statics.functions->adk_lock_events(); }
void adk_unlock_events() {  statics.functions->adk_unlock_events(); }
void adk_post_event(const adk_event_t event) {  statics.functions->adk_post_event(event); }
const char * adk_get_file_directory_path(const sb_file_directory_e directory) { return statics.functions->adk_get_file_directory_path(directory); }
size_t adk_get_screenshot_required_memory() { return statics.functions->adk_get_screenshot_required_memory(); }
void adk_take_screenshot_flush(image_t *const out_screenshot, const mem_region_t screenshot_mem_region) {  statics.functions->adk_take_screenshot_flush(out_screenshot, screenshot_mem_region); }
size_t adk_write_screenshot_mem_user_by_type(const image_t *const screenshot, const mem_region_t region, const image_save_file_type_e file_type) { return statics.functions->adk_write_screenshot_mem_user_by_type(screenshot, region, file_type); }
sb_locale_t sb_get_locale() { return statics.functions->sb_get_locale(); }
sb_getaddrinfo_result_t sb_getaddrinfo(const char *const address, const char *const port, const sb_addrinfo_t *const hint, sb_addrinfo_t *const out_addrinfos, uint32_t *const out_addrinfo_size) { return statics.functions->sb_getaddrinfo(address, port, hint, out_addrinfos, out_addrinfo_size); }
int sb_create_socket(const sb_socket_family_e domain, const sb_socket_type_e sock_type, const sb_socket_protocol_type_e protocol, sb_socket_t *const out_socket) { return statics.functions->sb_create_socket(domain, sock_type, protocol, out_socket); }
_Bool sb_enable_blocking_socket(const sb_socket_t sock, const sb_socket_blocking_e blocking_mode) { return statics.functions->sb_enable_blocking_socket(sock, blocking_mode); }
_Bool sb_socket_set_tcp_no_delay(const sb_socket_t sock, const sb_socket_tcp_delay_mode_e tcp_delay_mode) { return statics.functions->sb_socket_set_tcp_no_delay(sock, tcp_delay_mode); }
_Bool sb_socket_set_linger(const sb_socket_t sock, const sb_socket_linger_mode_e linger_mode, const int linger_timeout_in_seconds) { return statics.functions->sb_socket_set_linger(sock, linger_mode, linger_timeout_in_seconds); }
sb_socket_bind_result_t sb_bind_socket(const sb_socket_t sock, const sb_sockaddr_t *const addr) { return statics.functions->sb_bind_socket(sock, addr); }
sb_socket_listen_result_t sb_listen_socket(const sb_socket_t sock, const int backlog) { return statics.functions->sb_listen_socket(sock, backlog); }
sb_socket_accept_result_t sb_accept_socket(const sb_socket_t sock, sb_sockaddr_t *const optional_sockaddr, sb_socket_t *const out_sock) { return statics.functions->sb_accept_socket(sock, optional_sockaddr, out_sock); }
sb_socket_connect_result_t sb_connect_socket(const sb_socket_t sock, const sb_sockaddr_t *const addr) { return statics.functions->sb_connect_socket(sock, addr); }
void sb_close_socket(const sb_socket_t sock) {  statics.functions->sb_close_socket(sock); }
void sb_shutdown_socket(const sb_socket_t sock, const sb_socket_shutdown_flags_e shutdown_flags) {  statics.functions->sb_shutdown_socket(sock, shutdown_flags); }
sb_socket_select_result_e sb_socket_select(const sb_socket_t socket, milliseconds_t *const timeout) { return statics.functions->sb_socket_select(socket, timeout); }
sb_socket_receive_result_t sb_socket_receive(const sb_socket_t sock, const mem_region_t buffer, const sb_socket_receive_flags_e flags, int *const receive_size) { return statics.functions->sb_socket_receive(sock, buffer, flags, receive_size); }
sb_socket_receive_result_t sb_socket_receive_from(const sb_socket_t sock, const mem_region_t buffer, const sb_socket_receive_flags_e flags, sb_sockaddr_t *const out_addr, int *const receive_size) { return statics.functions->sb_socket_receive_from(sock, buffer, flags, out_addr, receive_size); }
sb_socket_send_result_t sb_socket_send(const sb_socket_t sock, const const_mem_region_t message, const sb_socket_send_flags_e flags, int *const bytes_sent) { return statics.functions->sb_socket_send(sock, message, flags, bytes_sent); }
sb_socket_send_result_t sb_socket_send_to(const sb_socket_t sock, const const_mem_region_t message, const sb_socket_send_flags_e flags, const sb_sockaddr_t *const addr, int *const bytes_sent) { return statics.functions->sb_socket_send_to(sock, message, flags, addr, bytes_sent); }
void sb_getnameinfo(const sb_sockaddr_t *const addr, mem_region_t *const host, mem_region_t *const service, const sb_getnameinfo_flags_e flags) {  statics.functions->sb_getnameinfo(addr, host, service, flags); }
void sb_getsockname(const sb_socket_t sock, sb_sockaddr_t *const sockaddr) {  statics.functions->sb_getsockname(sock, sockaddr); }
void sb_getpeername(const sb_socket_t sock, sb_sockaddr_t *const sockaddr) {  statics.functions->sb_getpeername(sock, sockaddr); }
const char * sb_getaddrinfo_error_str(const sb_getaddrinfo_error_code_e err) { return statics.functions->sb_getaddrinfo_error_str(err); }
const char * sb_socket_connect_error_str(const sb_socket_connect_error_e err) { return statics.functions->sb_socket_connect_error_str(err); }
const char * sb_socket_accept_error_str(const sb_socket_accept_error_e err) { return statics.functions->sb_socket_accept_error_str(err); }
const char * sb_socket_bind_error_str(const sb_socket_bind_error_e err) { return statics.functions->sb_socket_bind_error_str(err); }
const char * sb_socket_listen_error_str(const sb_socket_listen_error_e err) { return statics.functions->sb_socket_listen_error_str(err); }
const char * sb_socket_receive_error_str(const sb_socket_receive_error_e err) { return statics.functions->sb_socket_receive_error_str(err); }
const char * sb_socket_send_error_str(const sb_socket_send_error_e err) { return statics.functions->sb_socket_send_error_str(err); }
const char * sb_platform_socket_error_str(int err) { return statics.functions->sb_platform_socket_error_str(err); }
