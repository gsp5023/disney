/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
rbcmd.c

Render backend command support
*/

#include _PCH
#include "rhi.h"
#include "rhi_device_api.h"
#include "rhi_private.h"
#include "source/adk/renderer/renderer.h"

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

bool render_cmd_buf_write_render_command(rb_cmd_buf_t * const cmd_buf, const render_cmd_e cmd_id, const void * const cmd, const int alignment, const int size) {
    if (!render_cmd_buf_next_cmd_ptr(cmd_buf)) {
        return false;
    }

    // write the command before updating the cmd_ptr's so if we fail
    // we still have a valid command buffer
    void * const cmdblock = hlba_allocate_low(&cmd_buf->hlba, alignment, size);
    if (!cmdblock) {
        return false;
    }

    memcpy(cmdblock, cmd, size);

    // encode jump to next command
    if (cmd_buf->last_cmd_ptr) {
        const uint32_t jump = (uint32_t)(((uint8_t *)cmd_buf->next_cmd_ptr) - ((uint8_t *)cmd_buf->hlba.block));
        // set the jump table, make sure to mask off the old jump address
        // incase there is an old jump value set from a mark/unwind.
        cmd_buf->last_cmd_ptr[0] = (cmd_buf->last_cmd_ptr[0] & RB_CMD_ID_MASK) | (jump << NUM_RB_CMD_ID_BITS);
    }

    cmd_buf->last_cmd_ptr = cmd_buf->next_cmd_ptr;
    cmd_buf->next_cmd_ptr[0] = cmd_id;
    cmd_buf->next_cmd_ptr = NULL; // next operation will set this
    ++cmd_buf->num_cmds;

    return true;
}

/*
=======================================
s_rb_render_cmd_func_table

Use macros to construct a table of dispatch functions
to execute render commands. 
=======================================
*/

#ifdef ENABLE_RENDER_TAGS
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
        const TOKENPASTE(_enum, _t) * const cmd_args = (const TOKENPASTE(_enum, _t) *)FWD_ALIGN_PTR(cmd_ptr, ALIGN_OF(TOKENPASTE(_enum, _t))); \
        _body RB_RENDER_COMMAND_FUNC_RETURN_STATEMENT                                                                                          \
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

/*
=======================================
rb_cmd_buf_execute
=======================================
*/

void rb_cmd_buf_execute(rhi_device_t * device, rb_cmd_buf_t * const cmd_buf) {
    const uint8_t * const block_base = cmd_buf->hlba.block;
    const uint32_t * cmd_ptr = (const uint32_t *)block_base;
    int num_cmds = cmd_buf->num_cmds;

#ifdef ENABLE_RENDER_TAGS
    const char * last_cmd_tag = NULL;
    (void)last_cmd_tag;
#endif

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

#ifdef ENABLE_RENDER_TAGS
        last_cmd_tag =
#endif
            s_rb_render_cmd_func_table[cmd_id](device, cmd_ptr + 1);

        cmd_ptr = (const uint32_t *)(block_base + next_cmd_jump);
    }

    sb_atomic_fetch_add(&cmd_buf->retire_counter, 1, memory_order_relaxed);

    cmd_buf->next_cmd_ptr = NULL;
    cmd_buf->last_cmd_ptr = NULL;
    cmd_buf->num_cmds = 0;
    hlba_reset(&cmd_buf->hlba);
}
