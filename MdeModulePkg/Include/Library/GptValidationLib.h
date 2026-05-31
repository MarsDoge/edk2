/** @file
  Shared GPT validation helpers.

  These helpers validate untrusted GPT header and partition entry array
  metadata consistently across GPT producers/consumers.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef GPT_VALIDATION_LIB_H_
#define GPT_VALIDATION_LIB_H_

#include <Uefi.h>
#include <Protocol/BlockIo.h>
#include <Uefi/UefiGpt.h>

#define GPT_HEADER_REVISION_V1  0x00010000

/**
  Validate a GPT header against the supplied media geometry.

  The header CRC is verified over Header->Header.HeaderSize bytes. The CRC32
  field is restored before return.

  @param[in,out] Header         GPT header buffer read from media.
  @param[in]     Media          Block I/O media geometry.
  @param[in]     ExpectedMyLba  Expected value for Header->MyLBA.
  @param[out]    EntryArraySize Size in bytes of the GPT partition entry array.

  @retval EFI_SUCCESS            The GPT header is valid.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL or media is invalid.
  @retval EFI_DEVICE_ERROR       The GPT header is malformed or has bad CRC.
  @retval EFI_BAD_BUFFER_SIZE    The entry array size is zero or overflows UINT64.
**/
EFI_STATUS
EFIAPI
GptValidateHeader (
  IN OUT EFI_PARTITION_TABLE_HEADER  *Header,
  IN CONST EFI_BLOCK_IO_MEDIA        *Media,
  IN EFI_LBA                         ExpectedMyLba,
  OUT UINT64                         *EntryArraySize
  );

/**
  Return the validated byte size of the GPT partition entry array.

  @param[in]  Header          GPT header.
  @param[out] EntryArraySize  Size in bytes of the GPT partition entry array.

  @retval EFI_SUCCESS            The size is valid.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval EFI_BAD_BUFFER_SIZE    The size overflows UINT64 or is zero.
**/
EFI_STATUS
EFIAPI
GptGetPartitionEntryArraySize (
  IN CONST EFI_PARTITION_TABLE_HEADER  *Header,
  OUT UINT64                           *EntryArraySize
  );

/**
  Validate the GPT partition entry array CRC.

  @param[in] Header          Validated GPT header.
  @param[in] EntryArray      GPT partition entry array buffer.
  @param[in] EntryArraySize  Size in bytes of EntryArray.

  @retval EFI_SUCCESS            The entry array CRC is valid.
  @retval EFI_INVALID_PARAMETER  A required pointer is NULL.
  @retval EFI_BAD_BUFFER_SIZE    EntryArraySize does not match the header.
  @retval EFI_CRC_ERROR          The entry array CRC is invalid.
**/
EFI_STATUS
EFIAPI
GptValidatePartitionEntryArrayCrc (
  IN CONST EFI_PARTITION_TABLE_HEADER  *Header,
  IN CONST VOID                        *EntryArray,
  IN UINTN                             EntryArraySize
  );

#endif
