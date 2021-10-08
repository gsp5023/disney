/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
  private/log_receiver.c

  Core Logging

  Receiver sink for log messages sent via cnc_bus connection
  */

#include "source/adk/log/private/log_receiver.h"

#include "source/adk/cncbus/cncbus.h"
#include "source/adk/cncbus/cncbus_msg_types.h"
#include "source/adk/log/private/log_p.h"
#include "source/adk/steamboat/sb_file.h"

#include <time.h>

typedef enum tty_permitted_color_e {
    tty_color_default = 0,
    tty_color_red,
    tty_color_green,
    tty_color_yellow,
    tty_color_blue,
    tty_color_purple,
    tty_color_cyan,
    num_tty_permitted_colors
} tty_permitted_color_e;

static const uint8_t tty_permitted_color_attribute_values[num_tty_permitted_colors] = {
    0, // default
    31, // red
    32, // green
    33, // yellow
    34, // blue
    35, // purple
    36 // cyan
};

typedef struct tty_tag_options_t {
    uint32_t fourcc_tag;
    log_level_e minimum_log_level;
    uint8_t color_attribute_value;
} tty_tag_options_t;

enum {
    log_receiver_max_customized_tags = 8
};

static struct {
    cncbus_receiver_t bus_receiver;
    cncbus_receiver_vtable_t vtable;
    struct tty_options_t {
        bool show_location;
        bool colorize;
        tty_tag_options_t default_tag;
        tty_tag_options_t custom_tags[log_receiver_max_customized_tags];
    } tty_options;

} log_receiver = {0};

static int on_log_msg_received(cncbus_receiver_t * const self, const cncbus_msg_header_t header, cncbus_msg_t * const msg) {
    if (header.msg_type != cncbus_msg_type_log_v1) {
        return 1;
    }

    if (header.msg_size < sizeof(log_header_t)) {
        return 1;
    }

    // Read the log metadata into the struct
    log_header_t log_header;
    cncbus_msg_read(msg, &log_header, sizeof(log_header_t));

    // The rest of the cncbus msg is the text of the log message
    char log_msg[max_log_msg_length];
    int message_size = header.msg_size - sizeof(log_header_t);
    cncbus_msg_read(msg, log_msg, message_size);
    log_msg[message_size - 1] = 0;

    // TODO future, this is where file logging output will likely be inserted

    tty_tag_options_t * tty_tag_options;
    int i = 0;
    while (1) {
        if (log_receiver.tty_options.custom_tags[i].fourcc_tag == log_header.fourcc_tag) {
            tty_tag_options = &log_receiver.tty_options.custom_tags[i];
            break;
        }
        if ((log_receiver.tty_options.custom_tags[i].fourcc_tag == 0) || (++i == log_receiver_max_customized_tags)) {
            tty_tag_options = &log_receiver.tty_options.default_tag;
            break;
        }
    }

    if (log_header.level < tty_tag_options->minimum_log_level) {
        return 0;
    }

    struct tm time_info;
    sb_seconds_since_epoch_to_localtime(log_header.time_since_epoch.seconds, &time_info);

    char time_str[27];
    snprintf(time_str, ARRAY_SIZE(time_str), "%04d-%02d-%02d|%02d:%02d:%02d.%06d", time_info.tm_year + 1900, time_info.tm_mon + 1, time_info.tm_mday, time_info.tm_hour, time_info.tm_min, time_info.tm_sec, log_header.time_since_epoch.microseconds);

    if (log_receiver.tty_options.colorize) {
        const uint8_t color_attribute_value = (log_header.level == log_level_error) ? tty_permitted_color_attribute_values[tty_color_red] : tty_tag_options->color_attribute_value;
        if (log_receiver.tty_options.show_location) {
            debug_write_line("%s(%d): %s()\n\x1b[%dm[%s][%*.*s][%s] %s \x1b[0m", log_header.file, log_header.line, log_header.func, color_attribute_value, time_str, 4, 4, (char *)&log_header.fourcc_tag, log_get_level_short_name(log_header.level), log_msg);
        } else {
            debug_write_line("\x1b[%dm[%s][%*.*s][%s] %s \x1b[0m", color_attribute_value, time_str, 4, 4, (char *)&log_header.fourcc_tag, log_get_level_short_name(log_header.level), log_msg);
        }
    } else {
        if (log_receiver.tty_options.show_location) {
            debug_write_line("%s(%d): %s()\n[%s][%*.*s][%s] %s", log_header.file, log_header.line, log_header.func, time_str, 4, 4, (char *)&log_header.fourcc_tag, log_get_level_short_name(log_header.level), log_msg);
        } else {
            debug_write_line("[%s][%*.*s][%s] %s", time_str, 4, 4, (char *)&log_header.fourcc_tag, log_get_level_short_name(log_header.level), log_msg);
        }
    }

    return 0;
}

void log_receiver_init(cncbus_t * const bus, const cncbus_address_t address) {
    if (bus) {
#ifdef _SHIP
        log_receiver.tty_options.show_location = true;
#else
        log_receiver.tty_options.show_location = false;
#endif
        log_receiver.tty_options.colorize = true;
        log_receiver.tty_options.default_tag.color_attribute_value = tty_permitted_color_attribute_values[tty_color_default]; // TODO future, use cyan for default only until config file loading is added
        log_receiver.tty_options.default_tag.minimum_log_level = log_level_debug;
        ZEROMEM(&log_receiver.tty_options.custom_tags);

        log_receiver_reload_tty_options();

        log_receiver.vtable.on_msg_recv = on_log_msg_received;

        cncbus_init_receiver(&log_receiver.bus_receiver, &log_receiver.vtable, address);

        cncbus_connect(bus, &log_receiver.bus_receiver);
    }
}

void log_receiver_shutdown(cncbus_t * const bus) {
    if (bus) {
        cncbus_disconnect(bus, &log_receiver.bus_receiver);
    }
}

void log_receiver_reload_tty_options() {
    //sb_file_t * file = sb_fopen(sb_app_config_directory, "log.ini", "rt");
    //if (file) {
    //    // TODO future, read and apply config options from file

    //    sb_fclose(file);
    //}
}
