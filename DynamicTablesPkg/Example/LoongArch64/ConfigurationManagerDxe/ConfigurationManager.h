/** @file

  Platform data contract for the LoongArch64 MADT Configuration Manager
  example.

  Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>

#include <ConfigurationManagerObject.h>
#include <Protocol/MpService.h>

/** Map an MP Services processor record to the platform ACPI namespace.

  @param [in]  ProcessorNumber  MP Services processor number.
  @param [in]  PublishedIndex   Number of Core PIC objects already published.
  @param [in]  ProcessorInfo    MP Services processor information.
  @param [out] Publish          Whether to publish this processor.
  @param [out] ProcessorUid     Processor device _UID used by AML.
  @param [out] CoreId           Hardware Core ID used by the MADT.
  @param [out] Flags            Core PIC flags.

  @retval EFI_SUCCESS           The mapping was returned.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_UNSUPPORTED       The processor cannot be represented.
**/
typedef
EFI_STATUS
(EFIAPI *LOONGARCH64_MADT_MAP_PROCESSOR)(
  IN  UINTN                            ProcessorNumber,
  IN  UINT32                           PublishedIndex,
  IN  CONST EFI_PROCESSOR_INFORMATION  *ProcessorInfo,
  OUT BOOLEAN                          *Publish,
  OUT UINT32                           *ProcessorUid,
  OUT UINT32                           *CoreId,
  OUT UINT32                           *Flags
  );

/** Platform-owned MADT data consumed by the example Configuration Manager.

  The platform data provider owns every array for the lifetime of the
  Configuration Manager protocol. Core PIC objects are intentionally absent
  from this structure because they are built from EFI_MP_SERVICES_PROTOCOL.
**/
typedef struct LoongArch64MadtPlatformRepository {
  CM_LOONGARCH64_MADT_INFO          MadtInfo;
  LOONGARCH64_MADT_MAP_PROCESSOR    MapProcessor;
  UINT32                            MaximumProcessorCount;
  CM_LOONGARCH64_LIO_PIC_INFO       *LioPicInfo;
  UINT32                            LioPicCount;
  CM_LOONGARCH64_HT_PIC_INFO        *HtPicInfo;
  UINT32                            HtPicCount;
  CM_LOONGARCH64_EIO_PIC_INFO       *EioPicInfo;
  UINT32                            EioPicCount;
  CM_LOONGARCH64_MSI_PIC_INFO       *MsiPicInfo;
  UINT32                            MsiPicCount;
  CM_LOONGARCH64_BIO_PIC_INFO       *BioPicInfo;
  UINT32                            BioPicCount;
  CM_LOONGARCH64_LPC_PIC_INFO       *LpcPicInfo;
  UINT32                            LpcPicCount;
} LOONGARCH64_MADT_PLATFORM_REPOSITORY;

/** Return the platform-owned MADT repository.

  A real platform replaces PlatformMadtData.c with data derived from its
  node, bridge, and interrupt-controller topology.

  @retval A pointer to the platform-owned MADT repository.
**/
LOONGARCH64_MADT_PLATFORM_REPOSITORY *
GetLoongArch64MadtPlatformRepository (
  VOID
  );
