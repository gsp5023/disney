/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
bundle_zip_source.c

Uses steamboat file system to implement a read-only libzip "function source"
*/

#include "bundle_zip_source.h"

#include "bundle_zip_alloc.h"
#include "source/adk/runtime/runtime.h"

#include <errno.h>
#include <limits.h>

#define TAG_BUFS FOURCC('B', 'F', 'S', ' ')

// State information for our zip function source
// TODO cloned from libzip, some cleanup needed
typedef struct bundle_zip_source_ctx_t {
    zip_error_t error; // result of most recent command, for libzip error system
    sb_file_directory_e directory; // steamboat directory
    char fname[sb_max_path_length]; // steamboat file name
    size_t initial_offset; // offset (in bytes) for already-opened files
    sb_file_t * zipfile; // handle to zipfile, it open
    zip_stat_t stat; // cached stat
    zip_error_t stat_error; // cached stat result, for libzip error system
    zip_uint64_t len; // length of the file
    zip_uint64_t offset; // current seek offset
    bool eof; // true if eof reached on read
} bundle_zip_source_ctx_t;

// Map stdio seek mode to 'sb_seek_mode_e'
static const sb_seek_mode_e to_sb_seek_mode[] = {[SEEK_SET] = sb_seek_set, [SEEK_CUR] = sb_seek_cur, [SEEK_END] = sb_seek_end};

static zip_int64_t bundle_open_cmd_impl(bundle_zip_source_ctx_t * const ctx, void * const data, zip_uint64_t const len) {
    // Apparently libzip uses open command on an already-open source to reset it.  TODO Review libzip code to determine
    // if this logic is needed.
    if (!ctx->zipfile && ctx->fname[0]) {
        sb_stat_result_t stat = sb_stat(ctx->directory, ctx->fname);
        if (stat.error != sb_stat_success) {
            zip_error_set(&ctx->error, stat.error == sb_stat_error_no_entry ? ZIP_ER_NOENT : ZIP_ER_INTERNAL, 0);
            return -1;
        }
        ctx->zipfile = sb_fopen(ctx->directory, ctx->fname, "rb");
        if (!ctx->zipfile) {
            return -1;
        }

        sb_fseek(ctx->zipfile, (long)ctx->initial_offset, sb_seek_set);
    }

    ctx->offset = 0;
    ctx->eof = false;
    return 0;
}

static zip_int64_t bundle_read_cmd_impl(bundle_zip_source_ctx_t * const ctx, void * const data, zip_uint64_t const len) {
    if (ctx->eof) {
        return 0; // at eof, app can seek to reset
    }

    const size_t res = sb_fread(data, 1, len, ctx->zipfile);
    if (res < len) {
        ctx->eof = sb_feof(ctx->zipfile);
    }
    ctx->offset += res;

    return res;
}

static zip_int64_t bundle_close_cmd_impl(bundle_zip_source_ctx_t * const ctx, void * const data, zip_uint64_t const len) {
    if (ctx->zipfile) {
        sb_fclose(ctx->zipfile);
        ctx->zipfile = NULL;
    }
    return 0;
}

static zip_int64_t bundle_stat_cmd_impl(bundle_zip_source_ctx_t * const ctx, void * const data, zip_uint64_t const len) {
    if (len < sizeof(ctx->stat)) {
        zip_error_set(&ctx->error, ZIP_ER_INTERNAL, EINVAL); // libzip passed bad arg
        return -1;
    }

    if (zip_error_code_zip(&ctx->stat_error) != 0) {
        zip_error_set(&ctx->error, zip_error_code_zip(&ctx->stat_error), zip_error_code_system(&ctx->stat_error));
        return -1;
    }

    // TODO The libzip impl just returns a copy of the cached stat. That may not be necessary in our case, more
    // investigation needed.
    memcpy(data, &ctx->stat, sizeof(ctx->stat));
    return sizeof(ctx->stat);
}

static zip_int64_t bundle_error_cmd_impl(bundle_zip_source_ctx_t * const ctx, void * const data, zip_uint64_t const len) {
    return zip_error_to_data(&ctx->error, data, len);
}

static zip_int64_t bundle_free_cmd_impl(bundle_zip_source_ctx_t * const ctx, void * const data, zip_uint64_t const len) {
    if (ctx) {
        if (ctx->zipfile) {
            zip_close((zip_t *)ctx->zipfile);
        }
        zip_error_fini(&ctx->error);
        zip_error_fini(&ctx->stat_error);
        bundle_zip_free(ctx);
    }
    return 0;
}

static zip_int64_t bundle_seek_cmd_impl(bundle_zip_source_ctx_t * const ctx, void * const data, zip_uint64_t const len) {
    zip_int64_t new_offset = zip_source_seek_compute_offset(ctx->offset, ctx->len, data, len, &ctx->error);

    // The actual offset inside the zipfile must be representable as long.
    if ((new_offset < 0) || (new_offset > LONG_MAX)) {
        zip_error_set(&ctx->error, ZIP_ER_SEEK, EOVERFLOW);
        return -1;
    }

    if (!sb_fseek(ctx->zipfile, (long)(new_offset + ctx->initial_offset), sb_seek_set)) {
        zip_error_set(&ctx->error, ZIP_ER_SEEK, ERANGE); // assume seek past end
        return -1;
    }

    ctx->offset = (zip_uint64_t)new_offset;
    ctx->eof = false; // reset 'eof' per fseek() semantics

    return 0;
}

static zip_int64_t bundle_tell_cmd_impl(bundle_zip_source_ctx_t * const ctx, void * const data, zip_uint64_t const len) {
    return (zip_int64_t)ctx->offset;
}

static zip_int64_t bundle_accept_empty_cmd_impl(bundle_zip_source_ctx_t * const ctx, void * const data, zip_uint64_t const len) {
    return false;
}

static zip_int64_t bundle_supports_cmd_impl(bundle_zip_source_ctx_t * const ctx, void * const data, zip_uint64_t const len) {
    (void)data;
    (void)len;
    return ZIP_SOURCE_SUPPORTS_READABLE | ZIP_SOURCE_SUPPORTS_SEEKABLE;
}

// Place-holder for unimplimented commands
static zip_int64_t bundle_no_cmd_impl(bundle_zip_source_ctx_t * const ctx, void * const data, zip_uint64_t const len) {
    return -1;
}

typedef zip_int64_t (*bundle_zip_cmd_impl_t)(bundle_zip_source_ctx_t *, void *, zip_uint64_t);

// Implements zip_source_callback
static zip_int64_t bundle_zip_source_cb(void * ctx, void * data, zip_uint64_t len, zip_source_cmd_t cmd) {
#define NO_IMPL bundle_no_cmd_impl // eye candy
    static const bundle_zip_cmd_impl_t cmd_impls[ZIP_SOURCE_GET_FILE_ATTRIBUTES + 1] = {
        [ZIP_SOURCE_OPEN] = bundle_open_cmd_impl,
        [ZIP_SOURCE_READ] = bundle_read_cmd_impl,
        [ZIP_SOURCE_CLOSE] = bundle_close_cmd_impl,
        [ZIP_SOURCE_STAT] = bundle_stat_cmd_impl,
        [ZIP_SOURCE_ERROR] = bundle_error_cmd_impl,
        [ZIP_SOURCE_FREE] = bundle_free_cmd_impl,
        [ZIP_SOURCE_SEEK] = bundle_seek_cmd_impl,
        [ZIP_SOURCE_TELL] = bundle_tell_cmd_impl,
        [ZIP_SOURCE_BEGIN_WRITE] = NO_IMPL,
        [ZIP_SOURCE_COMMIT_WRITE] = NO_IMPL,
        [ZIP_SOURCE_ROLLBACK_WRITE] = NO_IMPL,
        [ZIP_SOURCE_WRITE] = NO_IMPL,
        [ZIP_SOURCE_SEEK_WRITE] = NO_IMPL,
        [ZIP_SOURCE_TELL_WRITE] = NO_IMPL,
        [ZIP_SOURCE_SUPPORTS] = bundle_supports_cmd_impl,
        [ZIP_SOURCE_REMOVE] = NO_IMPL,
        [ZIP_SOURCE_RESERVED_1] = NO_IMPL,
        [ZIP_SOURCE_BEGIN_WRITE_CLONING] = NO_IMPL,
        [ZIP_SOURCE_ACCEPT_EMPTY] = bundle_accept_empty_cmd_impl,
        [ZIP_SOURCE_GET_FILE_ATTRIBUTES] = NO_IMPL,
    };
#undef NO_IMPL

    VERIFY_MSG((unsigned)cmd < ARRAY_SIZE(cmd_impls), "Invalid zip_source_cmd_t %u", cmd);
    return cmd_impls[cmd]((bundle_zip_source_ctx_t *)ctx, data, len);
}

// Creates a new read-only zip function source on specified file
zip_source_t * bundle_zip_source_new(const sb_file_directory_e directory, const char * const fname, zip_error_t * const error) {
    if (!fname || !fname[0]) {
        zip_error_set(error, ZIP_ER_INVAL, 0);
        return NULL;
    }

    sb_stat_result_t res = sb_stat(directory, fname);
    if (res.error != sb_stat_success) {
        zip_error_set(error, ZIP_ER_READ, ENOENT);
        return NULL;
    }

    const int reqd_mode = sb_file_mode_readable | sb_file_mode_regular_file;
    if ((res.stat.mode & reqd_mode) != reqd_mode) {
        zip_error_set(error, ZIP_ER_NOZIP, EACCES);
        return NULL;
    }

    bundle_zip_source_ctx_t * ctx = bundle_zip_calloc(1, sizeof(*ctx));
    if (ctx) {
        zip_error_init(&ctx->error);
        ctx->directory = directory;
        strcpy_s(ctx->fname, sizeof(ctx->fname), fname);
        ctx->zipfile = NULL;

        zip_stat_init(&ctx->stat);
        ctx->stat.name = bundle_zip_strdup(ctx->fname);
        ctx->stat.size = res.stat.size;
        ctx->stat.mtime = res.stat.modification_time_s;
        ctx->stat.valid = ZIP_STAT_NAME | ZIP_STAT_SIZE | ZIP_STAT_MTIME;

        zip_error_init(&ctx->stat_error);

        ctx->len = res.stat.size;
        ctx->offset = 0;
        ctx->eof = false;

        zip_source_t * zs = zip_source_function_create(bundle_zip_source_cb, ctx, error);
        if (zs) {
            return zs;
        }
        bundle_zip_free(ctx);
    }

    return NULL;
}

// Creates a new read-only zipfile source on an open file
zip_source_t * bundle_zip_source_new_from_fp(sb_file_t * const file, size_t initial_offset, zip_error_t * const error) {
    bundle_zip_source_ctx_t * const ctx = bundle_zip_calloc(1, sizeof(*ctx));
    if (ctx) {
        zip_error_init(&ctx->error);
        ctx->zipfile = file; // save open file
        ctx->initial_offset = initial_offset;

        zip_stat_init(&ctx->stat);
        zip_error_init(&ctx->stat_error);

        // set zipfile length for libzip, can't open without it
        if (sb_fseek(file, 0, sb_seek_end)) {
            ctx->stat.size = (zip_uint64_t)sb_ftell(file) - ctx->initial_offset;
            ctx->stat.valid = ZIP_STAT_SIZE;
            ctx->len = ctx->stat.size;

            zip_source_t * const zs = zip_source_function_create(bundle_zip_source_cb, ctx, error);
            if (zs) {
                return zs;
            }
        }
        bundle_zip_free(ctx);
    }

    return NULL;
}
