/** @file
  Lzma Custom decompress algorithm Guid definition.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __LZMA_DECOMPRESS_GUID_H__
#define __LZMA_DECOMPRESS_GUID_H__

///
/// The Global ID used to identify a section of an FFS file of type
/// EFI_SECTION_GUID_DEFINED, whose contents have been compressed using LZMA.
///
#define LZMA_CUSTOM_DECOMPRESS_GUID  \
  { 0xEE4E5898, 0x3914, 0x4259, { 0x9D, 0x6E, 0xDC, 0x7B, 0xD7, 0x94, 0x03, 0xCF } }

///
/// The Global ID used to identify a section of an FFS file of type
/// EFI_SECTION_GUID_DEFINED, whose contents have been compressed using LZMA with X86 code Converter.
///
#define LZMAF86_CUSTOM_DECOMPRESS_GUID  \
  { 0xD42AE6BD, 0x1352, 0x4bfb, { 0x90, 0x9A, 0xCA, 0x72, 0xA6, 0xEA, 0xE8, 0x89 } }

extern GUID  gLzmaCustomDecompressGuid;
extern GUID  gLzmaF86CustomDecompressGuid;

// Experimental format pinned to XZ PR173 e74f96fd.
#define LZMA_LOONGARCH_CUSTOM_DECOMPRESS_GUID  \
  { 0x6E11988C, 0xBC47, 0x45B6, { 0xA9, 0xDC, 0x23, 0x89, 0x83, 0x43, 0x2D, 0x70 } }
extern GUID  gLzmaLoongArchCustomDecompressGuid;

#endif
