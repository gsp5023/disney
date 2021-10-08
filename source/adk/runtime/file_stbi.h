/* ===========================================================================
 *
 * Copyright (c) 2019-2020 Disney Streaming Technology LLC. All rights reserved.
 *
 * ==========================================================================*/

/*
file_stbi.h

stbi io support
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int adk_stbi_fread(void * const user, char * data, const int size);
void adk_stbi_fwrite(void * const user, void * data, const int size);
void adk_stbi_fseek(void * const user, const int n);
int adk_stbi_feof(void * const user);

#ifdef __cplusplus
}
#endif
