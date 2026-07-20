/** @file

  Representative Loongson MADT platform data.

  This file models the controller classes emitted by the existing Loongson
  MADT path. The values describe a single-node system with up to four logical
  processors and one I/O bridge. They are not a board support package. A
  platform must replace them with data derived from its own node and bridge
  topology.

  Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi65.h>

#include "ConfigurationManager.h"

#define LOONGARCH64_PIC_VERSION  1

/** Map one MP Services record to the sample platform ACPI namespace. **/
STATIC
EFI_STATUS
EFIAPI
MapLoongsonProcessor (
  IN  UINTN                            ProcessorNumber,
  IN  UINT32                           PublishedIndex,
  IN  CONST EFI_PROCESSOR_INFORMATION  *ProcessorInfo,
  OUT BOOLEAN                          *Publish,
  OUT UINT32                           *ProcessorUid,
  OUT UINT32                           *CoreId,
  OUT UINT32                           *Flags
  )
{
  if ((ProcessorInfo == NULL) || (Publish == NULL) ||
      (ProcessorUid == NULL) || (CoreId == NULL) || (Flags == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *Publish = FALSE;

  //
  // This filter is a sample-platform policy, not an MP Services rule.
  //
  if ((ProcessorInfo->ProcessorId == 0) &&
      ((ProcessorInfo->StatusFlag & PROCESSOR_AS_BSP_BIT) == 0))
  {
    return EFI_SUCCESS;
  }

  if ((ProcessorInfo->ProcessorId > MAX_UINT32) ||
      (PublishedIndex == MAX_UINT32))
  {
    return EFI_UNSUPPORTED;
  }

  //
  // The sample AML contract uses a one-based filtered processor ordinal.
  // A real platform mapper must return the _UID used by its processor devices.
  //
  *ProcessorUid = PublishedIndex + 1;
  *CoreId       = (UINT32)ProcessorInfo->ProcessorId;
  *Flags        = ((ProcessorInfo->StatusFlag & PROCESSOR_ENABLED_BIT) != 0) ?
                  BIT0 : 0;
  *Publish = TRUE;

  return EFI_SUCCESS;
}

STATIC CM_LOONGARCH64_LIO_PIC_INFO  mLioPicInfo[] = {
  {
    LOONGARCH64_PIC_VERSION,
    0x1FE01400,
    0x80,
    { 0x02,       0x00       },
    { 0x00FFFFFF, 0x00000000 }
  }
};

STATIC CM_LOONGARCH64_EIO_PIC_INFO  mEioPicInfo[] = {
  {
    LOONGARCH64_PIC_VERSION,
    0x03,
    0,
    BIT0
  }
};

STATIC CM_LOONGARCH64_MSI_PIC_INFO  mMsiPicInfo[] = {
  {
    LOONGARCH64_PIC_VERSION,
    0x2FF00000,
    0x40,
    0xC0
  }
};

STATIC CM_LOONGARCH64_BIO_PIC_INFO  mBioPicInfo[] = {
  {
    LOONGARCH64_PIC_VERSION,
    0xE0010000000ULL,
    0x1000,
    0,
    0x40
  }
};

STATIC CM_LOONGARCH64_LPC_PIC_INFO  mLpcPicInfo[] = {
  {
    LOONGARCH64_PIC_VERSION,
    0x10002000,
    0x1000,
    0x13
  }
};

STATIC LOONGARCH64_MADT_PLATFORM_REPOSITORY  mPlatformRepository = {
  {
    0x1FE01400,
    EFI_ACPI_6_5_PCAT_COMPAT
  },
  MapLoongsonProcessor,
  4,
  mLioPicInfo,
  ARRAY_SIZE (mLioPicInfo),
  NULL,
  0,
  mEioPicInfo,
  ARRAY_SIZE (mEioPicInfo),
  mMsiPicInfo,
  ARRAY_SIZE (mMsiPicInfo),
  mBioPicInfo,
  ARRAY_SIZE (mBioPicInfo),
  mLpcPicInfo,
  ARRAY_SIZE (mLpcPicInfo)
};

/** Return the platform-owned MADT repository. **/
LOONGARCH64_MADT_PLATFORM_REPOSITORY *
GetLoongArch64MadtPlatformRepository (
  VOID
  )
{
  return &mPlatformRepository;
}
