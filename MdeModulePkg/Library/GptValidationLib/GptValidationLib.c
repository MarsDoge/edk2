/** @file
  Shared GPT validation helpers.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/GptValidationLib.h>

/**
  Return TRUE if Value is a power of two.
**/
STATIC
BOOLEAN
GptIsPowerOfTwo (
  IN UINT32  Value
  )
{
  return (BOOLEAN)((Value != 0) && ((Value & (Value - 1)) == 0));
}

/**
  Validate the GPT header CRC.
**/
STATIC
EFI_STATUS
GptValidateHeaderCrc (
  IN OUT EFI_PARTITION_TABLE_HEADER  *Header
  )
{
  UINT32      Crc;
  UINT32      SavedCrc;

  SavedCrc             = Header->Header.CRC32;
  Header->Header.CRC32 = 0;
  Crc                  = CalculateCrc32 ((UINT8 *)Header, Header->Header.HeaderSize);
  Header->Header.CRC32 = SavedCrc;

  if (SavedCrc != Crc) {
    DEBUG ((DEBUG_ERROR, "Invalid GPT header CRC32\n"));
    return EFI_CRC_ERROR;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GptGetPartitionEntryArraySize (
  IN CONST EFI_PARTITION_TABLE_HEADER  *Header,
  OUT UINT64                           *EntryArraySize
  )
{
  UINT64  Size;

  if ((Header == NULL) || (EntryArraySize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Size = MultU64x32 (Header->NumberOfPartitionEntries, Header->SizeOfPartitionEntry);
  if (Size == 0) {
    DEBUG ((DEBUG_ERROR, "Invalid GPT partition entry array size\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  *EntryArraySize = Size;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GptValidateHeader (
  IN OUT EFI_PARTITION_TABLE_HEADER  *Header,
  IN CONST EFI_BLOCK_IO_MEDIA        *Media,
  IN EFI_LBA                         ExpectedMyLba,
  OUT UINT64                         *EntryArraySize
  )
{
  EFI_STATUS  Status;
  UINT64      LocalEntryArraySize;
  UINT64      EntryArrayBlocks;
  EFI_LBA     EntryArrayLastLba;

  if ((Header == NULL) || (Media == NULL) || (EntryArraySize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Media->BlockSize == 0) || (Media->LastBlock < PRIMARY_PART_HEADER_LBA)) {
    DEBUG ((DEBUG_ERROR, "Invalid GPT media geometry\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (Header->Header.Signature != EFI_PTAB_HEADER_ID) {
    DEBUG ((DEBUG_ERROR, "Invalid GPT header signature\n"));
    return EFI_DEVICE_ERROR;
  }

  if (Header->Header.Revision != GPT_HEADER_REVISION_V1) {
    DEBUG ((DEBUG_ERROR, "Invalid GPT header revision\n"));
    return EFI_DEVICE_ERROR;
  }

  if ((Header->Header.HeaderSize < sizeof (EFI_PARTITION_TABLE_HEADER)) ||
      (Header->Header.HeaderSize > Media->BlockSize))
  {
    DEBUG ((DEBUG_ERROR, "Invalid GPT header size\n"));
    return EFI_DEVICE_ERROR;
  }

  Status = GptValidateHeaderCrc (Header);
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if ((Header->MyLBA != ExpectedMyLba) ||
      (Header->MyLBA > Media->LastBlock) ||
      (Header->AlternateLBA > Media->LastBlock) ||
      (Header->FirstUsableLBA > Header->LastUsableLBA) ||
      (Header->LastUsableLBA > Media->LastBlock))
  {
    DEBUG ((DEBUG_ERROR, "Invalid GPT header LBA fields\n"));
    return EFI_DEVICE_ERROR;
  }

  if ((Header->NumberOfPartitionEntries == 0) ||
      (Header->SizeOfPartitionEntry < sizeof (EFI_PARTITION_ENTRY)) ||
      !GptIsPowerOfTwo (Header->SizeOfPartitionEntry))
  {
    DEBUG ((DEBUG_ERROR, "Invalid GPT partition entry metadata\n"));
    return EFI_DEVICE_ERROR;
  }

  Status = GptGetPartitionEntryArraySize (Header, &LocalEntryArraySize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (LocalEntryArraySize > MAX_UINT64 - Media->BlockSize + 1) {
    DEBUG ((DEBUG_ERROR, "GPT partition entry array size overflows block rounding\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  EntryArrayBlocks = DivU64x32 (
                       LocalEntryArraySize + Media->BlockSize - 1,
                       Media->BlockSize
                       );
  if ((Header->PartitionEntryLBA > Media->LastBlock) ||
      (EntryArrayBlocks == 0) ||
      (EntryArrayBlocks - 1 > Media->LastBlock - Header->PartitionEntryLBA))
  {
    DEBUG ((DEBUG_ERROR, "GPT partition entry array exceeds media bounds\n"));
    return EFI_DEVICE_ERROR;
  }

  EntryArrayLastLba = Header->PartitionEntryLBA + EntryArrayBlocks - 1;
  if ((Header->PartitionEntryLBA <= Header->LastUsableLBA) &&
      (EntryArrayLastLba >= Header->FirstUsableLBA))
  {
    DEBUG ((DEBUG_ERROR, "GPT partition entry array overlaps usable LBAs\n"));
    return EFI_DEVICE_ERROR;
  }

  *EntryArraySize = LocalEntryArraySize;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GptValidatePartitionEntryArrayCrc (
  IN CONST EFI_PARTITION_TABLE_HEADER  *Header,
  IN CONST VOID                        *EntryArray,
  IN UINTN                             EntryArraySize
  )
{
  EFI_STATUS  Status;
  UINT64      ExpectedSize;
  UINT32      Crc;

  if ((Header == NULL) || (EntryArray == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GptGetPartitionEntryArraySize (Header, &ExpectedSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((UINT64)EntryArraySize != ExpectedSize) {
    DEBUG ((DEBUG_ERROR, "Unexpected GPT partition entry array size\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  Crc = CalculateCrc32 ((VOID *)(UINTN)EntryArray, EntryArraySize);

  if (Header->PartitionEntryArrayCRC32 != Crc) {
    DEBUG ((DEBUG_ERROR, "Invalid GPT partition entry array CRC32\n"));
    return EFI_CRC_ERROR;
  }

  return EFI_SUCCESS;
}
