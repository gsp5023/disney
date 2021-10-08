/*
  zip_source_file_steamboat.c -- read-only stdio file source implementation
  Copyright (C) 2020 Dieter Baron and Thomas Klausner

  This file is part of libzip, a library to manipulate ZIP archives.
  The authors can be contacted at <libzip@nih.at>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:
  1. Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
  3. The names of the authors may not be used to endorse or promote
     products derived from this software without specific prior
     written permission.

  THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "zipint.h"

#include "zip_source_file.h"
#include "zip_source_file_steamboat.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifdef _WIN32
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#endif

/* clang-format off */
static zip_source_file_operations_t ops_sb_read = {
    _zip_sb_op_close,
    NULL,
    NULL,
    NULL,
    NULL,
    _zip_sb_op_read,
    NULL,
    NULL,
    _zip_sb_op_seek,
    _zip_sb_op_stat,
    NULL,
    _zip_sb_op_tell,
    NULL
};
/* clang-format on */


ZIP_EXTERN zip_source_t *
zip_source_filep(zip_t *za, FILE *file, zip_uint64_t start, zip_int64_t len) {
    if (za == NULL) {
	return NULL;
    }

    return zip_source_filep_create(file, start, len, &za->error);
}


ZIP_EXTERN zip_source_t *
zip_source_filep_create(FILE *file, zip_uint64_t start, zip_int64_t length, zip_error_t *error) {
    if (file == NULL || length < -1) {
	zip_error_set(error, ZIP_ER_INVAL, 0);
	return NULL;
    }

    return zip_source_file_common_new(NULL, file, start, length, NULL, &ops_sb_read, NULL, error);
}


void
_zip_sb_op_close(zip_source_file_context_t *ctx) {
    sb_fclose((sb_file_t *)ctx->f);
}


zip_int64_t
_zip_sb_op_read(zip_source_file_context_t *ctx, void *buf, zip_uint64_t len) {
    size_t i;
    if (len > SIZE_MAX) {
	len = SIZE_MAX;
    }

    if ((i = sb_fread(buf, 1, (size_t)len, (sb_file_t *)ctx->f)) == 0) {
	if (!sb_feof((sb_file_t *)ctx->f)) {
	    zip_error_set(&ctx->error, ZIP_ER_READ, errno); // TODO errno?
	    return -1;
	}
    }

    return (zip_int64_t)i;
}

static const sb_seek_mode_e to_sb_seek_mode[] = {[SEEK_SET] = sb_seek_set, [SEEK_CUR] = sb_seek_cur, [SEEK_END] = sb_seek_end};

bool
_zip_sb_op_seek(zip_source_file_context_t *ctx, void *f, zip_int64_t offset, int whence) {
#if ZIP_FSEEK_MAX > ZIP_INT64_MAX
    if (offset > ZIP_FSEEK_MAX || offset < ZIP_FSEEK_MIN) {
	zip_error_set(&ctx->error, ZIP_ER_SEEK, EOVERFLOW);
	return false;
    }
#endif

    if (sb_fseek((sb_file_t *)f, (long)offset, to_sb_seek_mode[whence]) < 0) {
	zip_error_set(&ctx->error, ZIP_ER_SEEK, errno); // TODO errno?
	return false;
    }
    return true;
}


bool
_zip_sb_op_stat(zip_source_file_context_t *ctx, zip_source_file_stat_t *st) {
    sb_stat_result_t sb = { 0 };

    if (ctx->fname) {
	sb = sb_stat(sb_app_cache_directory, ctx->fname);
    }
    else {
	sb.error = sb_stat_error_no_entry;
    }

    if (sb.error != sb_stat_success) {
        if (sb.error == sb_stat_error_no_entry) {
            st->exists = false;
            return true;
        }
	zip_error_set(&ctx->error, ZIP_ER_READ, sb.error); // TODO map to errno
	return false;
    }

    st->size = (zip_uint64_t)sb.stat.size;
    st->mtime = (time_t)sb.stat.modification_time_s;

    st->regular_file = ((sb.stat.mode & sb_file_mode_regular_file) == sb_file_mode_regular_file);
    st->exists = true;

    /* We're using UNIX file API, even on Windows; thus, we supply external file attributes with Unix values. */
    /* TODO: This could be improved on Windows by providing Windows-specific file attributes */
    ctx->attributes.valid = ZIP_FILE_ATTRIBUTES_HOST_SYSTEM | ZIP_FILE_ATTRIBUTES_EXTERNAL_FILE_ATTRIBUTES;
    ctx->attributes.host_system = ZIP_OPSYS_UNIX;
    //ctx->attributes.external_file_attributes = (((zip_uint32_t)sb.st_mode) << 16) | ((sb.st_mode & sb_file_mode_writable) ? 0 : 1);
    ctx->attributes.external_file_attributes = (sb.stat.mode & sb_file_mode_writable) ? 0 : 1;

    return true;
}

zip_int64_t
_zip_sb_op_tell(zip_source_file_context_t *ctx, void *f) {
    off_t offset = sb_ftell((sb_file_t *)f);

    if (offset < 0) {
	zip_error_set(&ctx->error, ZIP_ER_SEEK, errno);
    }

    return offset;
}


/*
 * fopen replacement that sets the close-on-exec flag
 * some implementations support an fopen 'e' flag for that,
 * but e.g. macOS doesn't.
 */
sb_file_t *
_zip_fopen_close_on_exec(const char *name, bool writeable) {
    return sb_fopen(sb_app_cache_directory, name, writeable ? "r+b" : "rb");
}
