/** @file
  The library instance provides security service of TPM2 measure boot and
  Confidential Computing (CC) measure boot.

  Caution: This file requires additional review when modified.
  This library will have external input - PE/COFF image and GPT partition.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  This file will pull out the validation logic from the following functions, in an
  attempt to validate the untrusted input in the form of unit tests

  These are those functions:

  DxeTpm2MeasureBootLibImageRead() function will make sure the PE/COFF image content
  read is within the image buffer.

  Tcg2MeasureGptTable() function will receive untrusted GPT partition table, and parse
  partition data carefully.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <Uefi/UefiSpec.h>
#include <Library/SafeIntLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <Protocol/BlockIo.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/GptValidationLib.h>

#include "DxeTpm2MeasureBootLibSanitization.h"

/**
  This function will validate the EFI_PARTITION_TABLE_HEADER structure is safe to parse
  However this function will not attempt to verify the validity of the GPT partition
  It will check the following:
    - Signature
    - Revision
    - AlternateLBA
    - FirstUsableLBA
    - LastUsableLBA
    - PartitionEntryLBA
    - NumberOfPartitionEntries
    - SizeOfPartitionEntry
    - BlockIo

  @param[in] PrimaryHeader
    Pointer to the EFI_PARTITION_TABLE_HEADER structure.

  @param[in] BlockIo
    Pointer to the EFI_BLOCK_IO_PROTOCOL structure.

  @retval EFI_SUCCESS
    The EFI_PARTITION_TABLE_HEADER structure is valid.

  @retval EFI_INVALID_PARAMETER
    The EFI_PARTITION_TABLE_HEADER structure is invalid.
**/
EFI_STATUS
EFIAPI
Tpm2SanitizeEfiPartitionTableHeader (
  IN OUT EFI_PARTITION_TABLE_HEADER  *PrimaryHeader,
  IN CONST EFI_BLOCK_IO_PROTOCOL       *BlockIo
  )
{
  UINT64  PartitionEntryArraySize;

  //
  // Verify that the input parameters are safe to use
  //
  if (PrimaryHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "Invalid Partition Table Header!\n"));
    return EFI_INVALID_PARAMETER;
  }

  if ((BlockIo == NULL) || (BlockIo->Media == NULL)) {
    DEBUG ((DEBUG_ERROR, "Invalid BlockIo!\n"));
    return EFI_INVALID_PARAMETER;
  }

  return GptValidateHeader (
           PrimaryHeader,
           BlockIo->Media,
           PRIMARY_PART_HEADER_LBA,
           &PartitionEntryArraySize
           );
}

/**
 This function will validate that the allocation size from the primary header is sane
  It will check the following:
    - AllocationSize does not overflow

  @param[in] PrimaryHeader
    Pointer to the EFI_PARTITION_TABLE_HEADER structure.

  @param[out] AllocationSize
    Pointer to the allocation size.

  @retval EFI_SUCCESS
    The allocation size is valid.

  @retval EFI_OUT_OF_RESOURCES
    The allocation size is invalid.
**/
EFI_STATUS
EFIAPI
Tpm2SanitizePrimaryHeaderAllocationSize (
  IN CONST EFI_PARTITION_TABLE_HEADER  *PrimaryHeader,
  OUT UINT32                           *AllocationSize
  )
{
  EFI_STATUS  Status;
  UINT64      EntryArraySize;

  if (PrimaryHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (AllocationSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GptGetPartitionEntryArraySize (PrimaryHeader, &EntryArraySize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Allocation Size would have overflowed!\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  if (EntryArraySize > MAX_UINT32) {
    DEBUG ((DEBUG_ERROR, "Allocation Size would have overflowed!\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  *AllocationSize = (UINT32)EntryArraySize;
  return EFI_SUCCESS;
}

/**
  This function will validate that the Gpt Event Size calculated from the primary header is sane
  It will check the following:
    - EventSize does not overflow

  Important: This function includes the entire length of the allocated space, including
  (sizeof (EFI_TCG2_EVENT) - sizeof (Tcg2Event->Event)) . When hashing the buffer allocated with this
  size, the caller must subtract the size of the (sizeof (EFI_TCG2_EVENT) - sizeof (Tcg2Event->Event))
  from the size of the buffer before hashing.

  @param[in] PrimaryHeader - Pointer to the EFI_PARTITION_TABLE_HEADER structure.
  @param[in] NumberOfPartition - Number of partitions.
  @param[out] EventSize - Pointer to the event size.

  @retval EFI_SUCCESS
    The event size is valid.

  @retval EFI_OUT_OF_RESOURCES
    Overflow would have occurred.

  @retval EFI_INVALID_PARAMETER
    One of the passed parameters was invalid.
**/
EFI_STATUS
Tpm2SanitizePrimaryHeaderGptEventSize (
  IN  CONST EFI_PARTITION_TABLE_HEADER  *PrimaryHeader,
  IN  UINTN                             NumberOfPartition,
  OUT UINT32                            *EventSize
  )
{
  EFI_STATUS  Status;
  UINT32      SafeNumberOfPartitions;

  if (PrimaryHeader == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (EventSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We shouldn't even attempt to perform the multiplication if the number of partitions is greater than the maximum value of UINT32
  //
  Status = SafeUintnToUint32 (NumberOfPartition, &SafeNumberOfPartitions);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "NumberOfPartition would have overflowed!\n"));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Replacing logic:
  // (UINT32)(sizeof (EFI_GPT_DATA) - sizeof (GptData->Partitions) + NumberOfPartition * PrimaryHeader.SizeOfPartitionEntry);
  //
  Status = SafeUint32Mult (SafeNumberOfPartitions, PrimaryHeader->SizeOfPartitionEntry, EventSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Event Size would have overflowed!\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  //
  // Replacing logic:
  // *EventSize + sizeof (EFI_TCG2_EVENT) - sizeof (Tcg2Event->Event);
  //
  Status = SafeUint32Add (
             OFFSET_OF (EFI_TCG2_EVENT, Event) + OFFSET_OF (EFI_GPT_DATA, Partitions),
             *EventSize,
             EventSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Event Size would have overflowed because of GPTData!\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}

/**
  This function will validate that the PeImage Event Size from the loaded image is sane
  It will check the following:
    - EventSize does not overflow

  @param[in] FilePathSize - Size of the file path.
  @param[out] EventSize - Pointer to the event size.

  @retval EFI_SUCCESS
    The event size is valid.

  @retval EFI_OUT_OF_RESOURCES
    Overflow would have occurred.

  @retval EFI_INVALID_PARAMETER
    One of the passed parameters was invalid.
**/
EFI_STATUS
Tpm2SanitizePeImageEventSize (
  IN  UINT32  FilePathSize,
  OUT UINT32  *EventSize
  )
{
  EFI_STATUS  Status;

  // Replacing logic:
  // sizeof (*ImageLoad) - sizeof (ImageLoad->DevicePath) + FilePathSize;
  Status = SafeUint32Add (OFFSET_OF (EFI_IMAGE_LOAD_EVENT, DevicePath), FilePathSize, EventSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EventSize would overflow!\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  // Replacing logic:
  // EventSize + sizeof (EFI_TCG2_EVENT) - sizeof (Tcg2Event->Event)
  Status = SafeUint32Add (*EventSize, OFFSET_OF (EFI_TCG2_EVENT, Event), EventSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "EventSize would overflow!\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}
