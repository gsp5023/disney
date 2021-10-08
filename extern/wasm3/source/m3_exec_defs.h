// Modifications to this software (c) 2021 Disney

//
//  m3_exec_defs.h
//
//  Created by Steven Massey on 5/1/19.
//  Copyright (c) 2019 Steven Massey. All rights reserved.
//

 // Modifications to this software (c) 2020-2021 Disney 

#ifndef m3_exec_defs_h
#define m3_exec_defs_h

#include "m3_core.h"
#include "m3_exec_ctx.h"

d_m3BeginExternC

#if d_m3HasFloat

#   define d_m3OpSig                pc_t _pc, m3stack_t _sp, M3MemoryHeader * _mem, m3reg_t _r0, f64 _fp0, m3_exec_ctx *ctx
#   define d_m3OpArgs               _sp, _mem, _r0, _fp0, ctx
#   define d_m3OpAllArgs            _pc, d_m3OpArgs
#   define d_m3OpDefaultArgs        0, 0.
#   define d_m3ClearRegisters       _r0 = 0; _fp0 = 0.;

#else

#   define d_m3OpSig                pc_t _pc, m3stack_t _sp, M3MemoryHeader * _mem, m3reg_t _r0, m3_exec_ctx *ctx
#   define d_m3OpArgs               _sp, _mem, _r0, ctx
#   define d_m3OpAllArgs            _pc, d_m3OpArgs
#   define d_m3OpDefaultArgs        0
#   define d_m3ClearRegisters       _r0 = 0;

#endif

#   define m3MemData(mem)           (u8*)(((M3MemoryHeader*)(mem))+1)
#   define m3MemRuntime(mem)        (((M3MemoryHeader*)(mem))->runtime)
#   define m3MemInfo(mem)           (&(((M3MemoryHeader*)(mem))->runtime->memory))

typedef m3ret_t (vectorcall * IM3Operation) (d_m3OpSig);

d_m3EndExternC

#endif // m3_exec_defs_h
