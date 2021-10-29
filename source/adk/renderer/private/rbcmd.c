/* ===========================================================================
 *
 * Copyright (c) 2019-2021 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
rbcmd.c

Render backend command support
*/

#include _PCH

#include "extern/stb/stb/stb_ds.h"
#include "rhi.h"
#include "rhi_device_api.h"
#include "rhi_private.h"
#include "source/adk/log/log.h"
#include "source/adk/renderer/renderer.h"
#include "source/adk/runtime/crc.h"
#include "source/adk/steamboat/sb_platform.h"

#define TAG_RBCMD FOURCC(' ', 'R', 'B', 'C')

typedef struct rhi_hashed_cmd_t {
    uint32_t hash;
    render_cmd_e cmd_id;
} rhi_hashed_cmd_t;

static struct {
    uint32_t hash;

    render_cmd_config_t config;

    struct {
        uint32_t matched_count;
        uint32_t mismatched_count;
    } metrics;

    struct {
        bool enabled;
        mem_region_t hash_commands_region;
        rhi_hashed_cmd_t * hash_commands;
        size_t hash_commands_length;
        bool is_commands_mismatched;
    } diff_tracking;

} statics;

void render_cmd_init(const render_cmd_config_t config) {
    statics.config = config;

    if (config.rhi_command_diffing.tracking.enabled) {
#ifndef _SHIP
        statics.diff_tracking.enabled = true;
        statics.diff_tracking.hash_commands_region = sb_map_pages(PAGE_ALIGN_PTR(sizeof(rhi_hashed_cmd_t) * config.rhi_command_diffing.tracking.buffer_size), system_page_protect_read_write);
        statics.diff_tracking.hash_commands = (rhi_hashed_cmd_t *)statics.diff_tracking.hash_commands_region.ptr;
#else
        LOG_WARN(TAG_RBCMD, "RHI command diff tracking is disallowed in SHIP builds");
#endif
    }
}

void render_cmd_shutdown() {
    if (statics.diff_tracking.enabled) {
        statics.diff_tracking.hash_commands = NULL;
        sb_unmap_pages(statics.diff_tracking.hash_commands_region);
    }
}

void render_cmd_log_metrics() {
    if (!statics.config.rhi_command_diffing.enabled || !statics.config.rhi_command_diffing.verbose) {
        return;
    }

    LOG_ALWAYS(TAG_RBCMD, "RHI diff: match: %lu, mismatch: %lu", statics.metrics.matched_count, statics.metrics.mismatched_count);
}

bool render_cmd_get_is_rhi_command_diffing_enabled() {
    return statics.config.rhi_command_diffing.enabled;
}

uint32_t render_cmd_shallow_hash(const void * const cmd, const int size) {
    return crc_32((const uint8_t *)cmd, size);
}

uint32_t render_cmd_random_hash(const void * const cmd, const int size) {
    static uint32_t counter = 0;
    return counter++;
}

uint32_t rbcmd_hash_bytes(const uint32_t seed, const uint8_t * bytes, const size_t num_bytes) {
    return (uint32_t)stbds_hash_bytes((void *)bytes, num_bytes, (size_t)seed);
}

/*
=======================================
render_cmd_buf_mark()
=======================================
*/

void render_mark_cmd_buf(rb_cmd_buf_t * const cmd_buf) {
    cmd_buf->mark.next_cmd_ptr = cmd_buf->next_cmd_ptr;
    cmd_buf->mark.last_cmd_ptr = cmd_buf->last_cmd_ptr;
    cmd_buf->mark.hlba_low = cmd_buf->hlba.ofs_low;
    cmd_buf->mark.hlba_high = cmd_buf->hlba.ofs_high;
    cmd_buf->mark.num_cmds = cmd_buf->num_cmds;
}

/*
=======================================
render_unwind_cmd_buf()
=======================================
*/

void render_unwind_cmd_buf(rb_cmd_buf_t * const cmd_buf) {
    cmd_buf->next_cmd_ptr = cmd_buf->mark.next_cmd_ptr;
    cmd_buf->last_cmd_ptr = cmd_buf->mark.last_cmd_ptr;
    cmd_buf->hlba.ofs_low = cmd_buf->mark.hlba_low;
    cmd_buf->hlba.ofs_high = cmd_buf->mark.hlba_high;
    cmd_buf->num_cmds = cmd_buf->mark.num_cmds;
}

/*
=======================================
render_cmd_buf_write_render_command()
=======================================
*/

#define RENDER_COMMAND_FUNC(_enum, _body) #_enum,
static const char * s_rb_render_cmd_name_table[num_render_commands] = {
#include "rbcmd_func.h"
};
#undef RENDER_COMMAND_FUNC

bool render_cmd_buf_write_render_command(
    rb_cmd_buf_t * const cmd_buf,
    const render_cmd_e cmd_id,
    const void * const cmd,
    const int alignment,
    const int size,
    render_cmd_hash_fn_t render_cmd_hash_fn) {
    if (!render_cmd_buf_next_cmd_ptr(cmd_buf)) {
        return false;
    }

    // write the command before updating the cmd_ptr's so if we fail
    // we still have a valid command buffer
    void * const cmdblock = hlba_allocate_low(&cmd_buf->hlba, alignment, size);
    if (!cmdblock) {
        return false;
    }

    RHI_TRACE_PUSH(s_rb_render_cmd_name_table[cmd_id]);

    memcpy(cmdblock, cmd, size);

    if (statics.config.rhi_command_diffing.enabled) {
        {
            RHI_TRACE_PUSH("rbcmd_hash_command");

            cmd_buf->hash = rbcmd_hash_bytes(cmd_buf->hash, (void *)&cmd_id, sizeof(cmd_id));

            const uint32_t hash = render_cmd_hash_fn(cmd, size);
            cmd_buf->hash = rbcmd_hash_bytes(cmd_buf->hash, (void *)&hash, sizeof(hash));

            RHI_TRACE_POP();
        }

        if (statics.diff_tracking.enabled) {
            if (!statics.diff_tracking.is_commands_mismatched && (size_t)cmd_buf->num_cmds < statics.diff_tracking.hash_commands_length) {
                rhi_hashed_cmd_t * const hashed_cmd = &statics.diff_tracking.hash_commands[cmd_buf->num_cmds];
                if (hashed_cmd->hash != cmd_buf->hash) {
                    statics.diff_tracking.is_commands_mismatched = true; // skip the remaining commands in this buffer
                    LOG_ALWAYS(TAG_RBCMD, "mismatched![%d] %s(%x) => %s(%x)", cmd_buf->num_cmds, s_rb_render_cmd_name_table[hashed_cmd->cmd_id], hashed_cmd->hash, s_rb_render_cmd_name_table[cmd_id], cmd_buf->hash);
                }
            }

            rhi_hashed_cmd_t * const hashed_cmd = &statics.diff_tracking.hash_commands[cmd_buf->num_cmds];
            hashed_cmd->hash = cmd_buf->hash;
            hashed_cmd->cmd_id = cmd_id;
        }
    }

    // encode jump to next command
    if (cmd_buf->last_cmd_ptr) {
        const uint32_t jump = (uint32_t)(((uint8_t *)cmd_buf->next_cmd_ptr) - ((uint8_t *)cmd_buf->hlba.block));
        // set the jump table, make sure to mask off the old jump address
        // in case there is an old jump value set from a mark/unwind.
        cmd_buf->last_cmd_ptr[0] = (cmd_buf->last_cmd_ptr[0] & RB_CMD_ID_MASK) | (jump << NUM_RB_CMD_ID_BITS);
    }

    cmd_buf->last_cmd_ptr = cmd_buf->next_cmd_ptr;
    cmd_buf->next_cmd_ptr[0] = cmd_id;
    cmd_buf->next_cmd_ptr = NULL; // next operation will set this
    ++cmd_buf->num_cmds;

    RHI_TRACE_POP();

    return true;
}

/*
=======================================
s_rb_render_cmd_func_table

Use macros to construct a table of dispatch functions
to execute render commands. 
=======================================
*/

#if ENABLE_RENDER_TAGS
#define RB_RENDER_COMMAND_FUNC_RETURN_TYPE const char *
#define RB_RENDER_COMMAND_FUNC_RETURN_STATEMENT return cmd_args->tag;
#else
#define RB_RENDER_COMMAND_FUNC_RETURN_TYPE void
#define RB_RENDER_COMMAND_FUNC_RETURN_STATEMENT
#endif

typedef RB_RENDER_COMMAND_FUNC_RETURN_TYPE (*rb_render_cmd_func)(rhi_device_t * const device, const void * const cmd_ptr);

/*
=======================================
Generate function bodies
=======================================
*/

#define RENDER_COMMAND_FUNC(_enum, _body)                                                                                                      \
    static RB_RENDER_COMMAND_FUNC_RETURN_TYPE TOKENPASTE(_enum, _cmd_func)(rhi_device_t * const device, const void * const cmd_ptr) {          \
        RHI_TRACE_PUSH(#_enum);                                                                                                                \
        const TOKENPASTE(_enum, _t) * const cmd_args = (const TOKENPASTE(_enum, _t) *)FWD_ALIGN_PTR(cmd_ptr, ALIGN_OF(TOKENPASTE(_enum, _t))); \
        (void)cmd_args;                                                                                                                        \
        _body RHI_TRACE_POP();                                                                                                                 \
        RB_RENDER_COMMAND_FUNC_RETURN_STATEMENT                                                                                                \
    }

#include "rbcmd_func.h"

#undef RENDER_COMMAND_FUNC

/*
=======================================
Build the table
=======================================
*/

#define RENDER_COMMAND_FUNC(_enum, _body) TOKENPASTE(_enum, _cmd_func),

static rb_render_cmd_func s_rb_render_cmd_func_table[num_render_commands] = {
#include "rbcmd_func.h"
};

#undef RENDER_COMMAND_FUNC

/*
=======================================
Static assert that the command funcs are in-order and correctly indexed
=======================================
*/

#define RENDER_COMMAND_FUNC(_enum, _body) TOKENPASTE(_enum, _check),

enum render_cmd_order_check_e {
#include "rbcmd_func.h"
};

#undef RENDER_COMMAND_FUNC
#define RENDER_COMMAND_FUNC(_enum, _body) STATIC_ASSERT(TOKENPASTE(_enum, _check) == (enum render_cmd_order_check_e)_enum);
#include "rbcmd_func.h"

#undef RENDER_COMMAND_FUNC

static bool rb_cmd_buf_test_hash(rb_cmd_buf_t * const cmd_buf, int num_cmds) {
    bool is_command_buffer_match = false;

    if (statics.config.rhi_command_diffing.enabled) {
        if (statics.diff_tracking.enabled) {
            statics.diff_tracking.hash_commands_length = num_cmds;
            statics.diff_tracking.is_commands_mismatched = false; // reset
        }

        is_command_buffer_match = statics.hash == cmd_buf->hash;

        if (is_command_buffer_match) {
            statics.metrics.matched_count += 1;
        } else {
            statics.metrics.mismatched_count += 1;
        }

        statics.hash = cmd_buf->hash;
        cmd_buf->hash = 0;
    }

    return is_command_buffer_match;
}

/*
=======================================
rb_cmd_buf_execute
=======================================
*/

void rb_cmd_buf_execute(rhi_device_t * device, rb_cmd_buf_t * const cmd_buf) {
    const uint8_t * const block_base = cmd_buf->hlba.block;
    const uint32_t * cmd_ptr = (const uint32_t *)block_base;
    int num_cmds = cmd_buf->num_cmds;

    const bool is_command_buffer_match = rb_cmd_buf_test_hash(cmd_buf, num_cmds);

#if ENABLE_RENDER_TAGS
    const char * last_cmd_tag = NULL;
    (void)last_cmd_tag;
#endif

    if (!is_command_buffer_match) {
        while (num_cmds-- > 0) {
            const render_cmd_e cmd_id = cmd_ptr[0] & RB_CMD_ID_MASK;
            ASSERT(cmd_id >= 0);
            ASSERT(cmd_id < num_render_commands);
            ASSERT(s_rb_render_cmd_func_table[cmd_id]);

            const int next_cmd_jump = (cmd_ptr[0] >> NUM_RB_CMD_ID_BITS) & MAX_RB_CMD_JUMP_MASK;

            // next_cmd_jump may be non-zero even if this is the last command
            // because we unwound the buffer, see rb_cmd_buf_unwind()

            ASSERT(next_cmd_jump >= 0);
            ASSERT(next_cmd_jump < cmd_buf->hlba.size);

#if ENABLE_RENDER_TAGS
            last_cmd_tag =
#endif
                s_rb_render_cmd_func_table[cmd_id](device, cmd_ptr + 1);

            cmd_ptr = (const uint32_t *)(block_base + next_cmd_jump);
        }
    }

    sb_atomic_fetch_add(&cmd_buf->retire_counter, 1, memory_order_relaxed);

    cmd_buf->next_cmd_ptr = NULL;
    cmd_buf->last_cmd_ptr = NULL;
    cmd_buf->num_cmds = 0;
    hlba_reset(&cmd_buf->hlba);
}
