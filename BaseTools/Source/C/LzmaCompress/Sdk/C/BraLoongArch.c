// SPDX-License-Identifier: 0BSD
// Experimental adaptation of XZ PR173, commit
// e74f96fd3e6174e1424357b0e8351e0671bf59ab.
// https://github.com/tukaani-project/xz/pull/173
// Whole-buffer format: call once at offset zero on both sides.
// Keep the entire modified buffer, including bytes past the returned prefix.

///////////////////////////////////////////////////////////////////////////////
//
/// \file       loongarch.c
/// \brief      Filter for loongarch binaries
///
/// This converts loongarch relative addresses in the BL, PCALAU12I. PCADDU18I
/// immediates to absolute values to increase redundancy of loongarch code.
///
/// PCADDI is also converted. PCADDU12I and B are left unchanged.
//
//  Authors:    Lasse Collin
//
///////////////////////////////////////////////////////////////////////////////

#include "BraLoongArch.h"
#include "CpuArch.h"

// This macro checks multiple conditions:
// (1) inst2 is JIRL
// (2) JIRL's rj equals to PCADDU18I's rd
// (3) JIRL's rd = $ra/$zero
// These conditions could be found in laelf.adoc
// we use them to further reduce false-positives.
#define NOT_PCADDU18I_PAIR(pcaddu18i, jirl) \
        (((jirl >> 26) != 0x13) \
        || ((pcaddu18i & 0x1F) != ((jirl >> 5) & 0x1F)) \
        || ((jirl & 0x1F) != 0x1 && (jirl & 0x1F) != 0x0))

SizeT
LoongArch_Convert (
  Byte    *buffer,
  SizeT   size,
  UInt32  now_pos,
  int     is_encoder
  )
{
  if (size < 12) {
    return 0;
  }

  size -= 12;

  SizeT  i;

  for (i = 0; i <= size; i += 4) {
    UInt32  pc   = (UInt32)(now_pos + i + 4);
    UInt32  inst = GetUi32 (buffer + i + 4);

    if ((inst >> 26) == 0x15) {
      // BL instruction:
      // The full 26-bit immediate is converted.
      // The range is +/-128 MiB.
      //
      // Using the full range helps quite a lot with
      // big executables. Smaller range would reduce false
      // positives in non-code sections of the input though
      // so this is a compromise that slightly favors big
      // files. With the full range, only six bits of the 32
      // need to match to trigger a conversion.
      const UInt32  src = ((inst & 0x3FF) << 16)
                          | ((inst & 0x3FFFFFF) >> 10);

      // We use previous instruction of the BL to reduce
      // false positives while filtering non-code buffer,
      // the previous instructions we use below are get
      // by analyze loongarch executables and libraries.
      UInt32  prev_inst = GetUi32 (buffer + i);

      // The instructions below are (in opcode order):
      // 1. OR ~ LD.WU
      // 2. BEQZ, BNEZ, B, BL
      // Only if prev_inst are in these instructions,
      // we'll convert BL's immediate.
      if (  ((prev_inst - 0x00150000) > 0x2A800000 - 0x00150000)
         && ((prev_inst & 0xE8000000) != 0x40000000))
      {
        continue;
      }

      inst = 0x54000000;

      pc >>= 2;
      if (!is_encoder) {
        pc = 0U - pc;
      }

      const UInt32  dest = src + pc;
      inst |= (dest & 0xFFFF) << 10;
      inst |= (dest & 0x3FF0000) >> 16;
      SetUi32 (buffer + i + 4, inst);
    } else if ((inst >> 25) == 0xC) {
      // PCADDI instruction:
      // We use the previous instruction again to avoid
      // false-positives.
      const UInt32  src = (inst >> 5) & 0xFFFFF;
      inst &= 0x1800001F;

      UInt32  prev_inst = GetUi32 (buffer + i);

      if (  ((prev_inst - 0x00150000) > 0x2A800000 - 0x00150000)
         && ((prev_inst & 0xE8000000) != 0x40000000))
      {
        continue;
      }

      pc >>= 2;
      if (!is_encoder) {
        pc = 0U - pc;
      }

      const UInt32  dest = src + pc;
      inst |= (dest & 0xFFFFF) << 5;
      inst |= (0U - (dest & 0x80000)) & 0x1000000;
      SetUi32 (buffer + i + 4, inst);
    } else if ((inst >> 25) == 0xD) {
      // PCALAU12I instruction:
      // Only values in range +/-512MB are converted.
      //
      // Similar to ARM64's ADRP instruction, using
      // less than the full range reduces falses
      // positives on non-code sections of the input.
      const UInt32  src = ((inst >> 5) & 0xFFFFF);

      if ((src + 0x20000) & 0xC0000) {
        continue;
      }

      inst &= 0x1A00001F;

      pc >>= 12;
      if (!is_encoder) {
        pc = 0U - pc;
      }

      const UInt32  dest = src + pc;
      inst |= (dest & 0x3FFFF) << 5;
      inst |= (0U - (dest & 0x20000)) & 0x1800000;
      SetUi32 (buffer + i + 4, inst);
    } else if ((inst >> 25) == 0xF) {
      // PCADDU18I + JIRL pair
      // According to laelf.adoc and elfnn-loongarch.c in
      // binutils this instruction pair is used for medium
      // code model only, so detect them as a whole to reduce
      // false-positives. And because the original PCADDU18I
      // shift the immediate by 18-bit and sign-extend, so we
      // have to use UInt64 to store address.
      //
      // Also, due to all loongarch instrcutions are 4 bytes
      // long, we could just shift the pc by 2 bits instead
      // of shift JIRL or PCADDU18I's immediate by 18 or
      // 2 bits, so that the whole address would fit into
      // 36 bits instead of 38 bits.
      UInt32  inst2 = GetUi32 (buffer + i + 8);
      UInt64  addr  = 0;

      if (is_encoder) {
        addr = (((UInt64)inst >> 5) & 0xFFFFF) << 16;
        if (addr & 0x800000000) {
          addr |= 0xFFFFFFF000000000;
        }

        if (NOT_PCADDU18I_PAIR (inst, inst2)) {
          continue;
        }

        UInt64  inst2_addr =
          ((UInt64)inst2 >> 10 & 0xFFFF);

        // Sign-extend the 16-bit immediate in JIRL.
        if (inst2_addr & 0x8000) {
          inst2_addr |= 0xFFFFFFFFFFFF0000;
        }

        addr += inst2_addr;

        // Skipping if addr equals 0 to improve
        // compress ratio of .o and .a files.
        if (addr == 0) {
          continue;
        }

        if (addr + (pc >> 2) == 0) {
          addr = 0;
        }

        addr += pc >> 2;

        inst &= 0x1E00001F;
        inst |= (UInt32)((addr & 0xFFFFF) << 5);

        inst2 &= 0xFC0003FF;
        inst2 |= (UInt32)(((addr >> 20) & 0xFFFF) << 10);

        SetUi32 (buffer + i + 4, inst);
        SetUi32 (buffer + i + 8, inst2);
      } else {
        if (NOT_PCADDU18I_PAIR (inst, inst2)) {
          continue;
        }

        addr = (((UInt64)inst >> 5) & 0xFFFFF) |
               ((UInt64)inst2 >> 10 & 0xFFFF) << 20;

        if (addr & 0x800000000) {
          addr |= 0xFFFFFFF000000000;
        }

        if (addr == 0) {
          continue;
        }

        if (addr == pc >> 2) {
          addr = 0;
        }

        addr -= pc >> 2;

        inst &= 0x1E00001F;
        inst |= (UInt32)((((addr + 0x8000) >> 16) << 5) & 0x1FFFFE0);

        inst2 &= 0xFC0003FF;
        inst2 |= (UInt32)((addr & 0x3FFFF) << 10);

        SetUi32 (buffer + i + 4, inst);
        SetUi32 (buffer + i + 8, inst2);
      }
    }
  }

  return i;
}
