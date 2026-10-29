/* SPDX-License-Identifier: 0BSD */

#ifndef BRA_LOONGARCH_H
#define BRA_LOONGARCH_H
#include "7zTypes.h"
EXTERN_C_BEGIN

/* Experimental whole-buffer BCJ format. Both sides use start offset zero.
   The returned prefix is NOT a bound on modified bytes. Preserve the full buffer. */
SizeT
LoongArch_Convert (
  Byte    *buffer,
  SizeT   size,
  UInt32  now_pos,
  int     is_encoder
  );

EXTERN_C_END
#endif
