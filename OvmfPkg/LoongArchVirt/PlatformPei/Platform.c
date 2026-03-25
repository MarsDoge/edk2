/** @file
  Platform PEI driver

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>
  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Mem - Memory
**/

//
// The package level header files this module uses
//
#include <PiPei.h>
//
// The Library classes this module consumes
//
#include <Library/BaseLib.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/FdtHob.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CpuMmuInitLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MpInitLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PlatformHookLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Ppi/MasterBootMode.h>
#include <Register/LoongArch64/Cpucfg.h>
#include <Register/LoongArch64/Csr.h>
#include <Uefi/UefiSpec.h>

#include "Platform.h"

STATIC EFI_MEMORY_TYPE_INFORMATION  mDefaultMemoryTypeInformation[] = {
  { EfiReservedMemoryType,  0x004 },
  { EfiRuntimeServicesData, 0x024 },
  { EfiRuntimeServicesCode, 0x030 },
  { EfiBootServicesCode,    0x180 },
  { EfiBootServicesData,    0xF00 },
  { EfiMaxMemoryType,       0x000 }
};

//
// Module globals
//
CONST EFI_PEI_PPI_DESCRIPTOR  mPpiListBootMode = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMasterBootModePpiGuid,
  NULL
};

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  mTpm2DiscoveredPpi = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gOvmfTpmDiscoveredPpiGuid,
  NULL
};

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  mTpm2InitializationDonePpi = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gPeiTpmInitializationDonePpiGuid,
  NULL
};

STATIC EFI_BOOT_MODE  mBootMode = BOOT_WITH_FULL_CONFIGURATION;

/**
  Create system type  memory range hand off block.

  @param  MemoryBase    memory base address.
  @param  MemoryLimit  memory length.

  @return  VOID
**/
STATIC
VOID
AddMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

/**
  Create  memory range hand off block.

  @param  MemoryBase    memory base address.
  @param  MemoryLimit  memory length.

  @return  VOID
**/
VOID
AddMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  EFI_PHYSICAL_ADDRESS  MemoryLimit
  )
{
  AddMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}

STATIC
VOID
SaveRtcRegisterAddressHob (
  UINT64  RtcRegisterBase
  )
{
  UINT64  Data64;

  //
  // Build location of RTC register base address buffer in HOB
  //
  Data64 = (UINT64)(UINTN)RtcRegisterBase;

  BuildGuidDataHob (
    &gRtcRegisterBaseAddressHobGuid,
    (VOID *)&Data64,
    sizeof (UINT64)
    );
}

STATIC
VOID
SetupTpmResources (
  IN CONST VOID  *Fdt
  )
{
  INT32         Node;
  INT32         Prev;
  INT32         Parent;
  INT32         Depth;
  INT32         Len;
  INT32         RangesLen;
  CONST CHAR8   *Compatible;
  CONST CHAR8   *CompItem;
  CONST UINT8   *RegProp;
  CONST UINT32  *RangesProp;
  UINT64        TpmBase;
  UINT64        TpmSize;
  EFI_STATUS    Status;

  TpmBase = 0;
  TpmSize = 0;
  Parent  = 0;

  for (Prev = Depth = 0; ; Prev = Node) {
    Node = FdtNextNode (Fdt, Prev, &Depth);
    if (Node < 0) {
      break;
    }

    if (Depth == 1) {
      Parent = Node;
    }

    Compatible = FdtGetProp (Fdt, Node, "compatible", &Len);
    for (CompItem = Compatible; CompItem != NULL && CompItem < Compatible + Len;
         CompItem += 1 + AsciiStrLen (CompItem))
    {
      if (AsciiStrCmp (CompItem, "tcg,tpm-tis-mmio") != 0) {
        continue;
      }

      RegProp = FdtGetProp (Fdt, Node, "reg", &Len);
      ASSERT (Len == 8 || Len == 16);
      if (Len == 8) {
        TpmBase = Fdt32ToCpu (*(CONST UINT32 *)RegProp);
        TpmSize = Fdt32ToCpu (*(CONST UINT32 *)(RegProp + sizeof (UINT32)));
      } else {
        TpmBase = Fdt64ToCpu (ReadUnaligned64 ((CONST UINT64 *)RegProp));
        TpmSize = Fdt64ToCpu (ReadUnaligned64 ((CONST UINT64 *)(RegProp + sizeof (UINT64))));
      }

      if (Depth > 1) {
        RangesProp = FdtGetProp (Fdt, Parent, "ranges", &RangesLen);
        ASSERT (RangesProp != NULL);

        if (RangesLen != 0) {
          if (RangesLen != Len + 2 * sizeof (UINT32)) {
            DEBUG ((
              DEBUG_WARN,
              "%a: 'ranges' property has unexpected size %d\n",
              __func__,
              RangesLen
              ));
            TpmBase = 0;
            TpmSize = 0;
            break;
          }

          if (Len == 8) {
            TpmBase -= Fdt32ToCpu (RangesProp[0]);
          } else {
            TpmBase -= Fdt64ToCpu (ReadUnaligned64 ((CONST UINT64 *)RangesProp));
          }

          RangesProp = (CONST UINT32 *)((CONST UINT8 *)RangesProp + Len / 2);
          TpmBase   += Fdt64ToCpu (ReadUnaligned64 ((CONST UINT64 *)RangesProp));
        }
      }

      break;
    }
  }

  if (TpmSize != 0) {
    BuildResourceDescriptorHob (
      EFI_RESOURCE_MEMORY_MAPPED_IO,
      EFI_RESOURCE_ATTRIBUTE_PRESENT     |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED,
      TpmBase,
      ALIGN_VALUE (TpmSize, EFI_PAGE_SIZE)
      );

    DEBUG ((DEBUG_INFO, "%a: TPM @ 0x%Lx size 0x%Lx\n", __func__, TpmBase, TpmSize));
    Status = (EFI_STATUS)PcdSet64S (PcdTpmBaseAddress, TpmBase);
    ASSERT_EFI_ERROR (Status);
    Status = PeiServicesInstallPpi (&mTpm2DiscoveredPpi);
  } else {
    Status = PeiServicesInstallPpi (&mTpm2InitializationDonePpi);
  }

  ASSERT_EFI_ERROR (Status);
}

/**
  Create  memory type information hand off block.

  @param  VOID

  @return  VOID
**/
STATIC
VOID
MemMapInitialization (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "==%a==\n", __func__));
  //
  // Create Memory Type Information HOB
  //
  BuildGuidDataHob (
    &gEfiMemoryTypeInformationGuid,
    mDefaultMemoryTypeInformation,
    sizeof (mDefaultMemoryTypeInformation)
    );
}

/** Get the Rtc base address from the DT.

  This function fetches the node referenced in the "loongson,ls7a-rtc"
  property of the "reg" node and returns the base address of
  the RTC.

  @param [in]   Fdt                   Pointer to a Flattened Device Tree (Fdt).
  @param [out]  RtcBaseAddress  If success, contains the base address
                                      of the Rtc.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_NOT_FOUND           RTC info not found in DT.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GetRtcAddress (
  IN  CONST VOID    *Fdt,
  OUT       UINT64  *RtcBaseAddress
  )
{
  INT32         Node;
  INT32         Prev;
  CONST CHAR8   *Type;
  INT32         Len;
  CONST UINT64  *RegProp;
  EFI_STATUS    Status;

  if ((Fdt == NULL) || (FdtCheckHeader (Fdt) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_NOT_FOUND;
  for (Prev = 0; ; Prev = Node) {
    Node = FdtNextNode (Fdt, Prev, NULL);
    if (Node < 0) {
      break;
    }

    //
    // Check for memory node
    //
    Type = FdtGetProp (Fdt, Node, "compatible", &Len);
    if ((Type) && (AsciiStrnCmp (Type, "loongson,ls7a-rtc", Len) == 0)) {
      //
      // Get the 'reg' property of this node. For now, we will assume
      // two 8 byte quantities for base and size, respectively.
      //
      RegProp = FdtGetProp (Fdt, Node, "reg", &Len);
      if ((RegProp != 0) && (Len == (2 * sizeof (UINT64)))) {
        *RtcBaseAddress = SwapBytes64 (RegProp[0]);
        Status          = RETURN_SUCCESS;
        DEBUG ((DEBUG_INFO, "%a Len %d RtcBase %llx\n", __func__, Len, *RtcBaseAddress));
        break;
      } else {
        DEBUG ((DEBUG_ERROR, "%a: Failed to parse FDT rtc node\n", __func__));
        break;
      }
    }
  }

  return Status;
}

/**
  Misc Initialization.

  @param  VOID

  @return  VOID
**/
STATIC
VOID
MiscInitialization (
  VOID
  )
{
  CPUCFG_REG1_INFO_DATA  CpucfgReg1Data;
  UINT8                  CpuPhysMemAddressWidth;

  DEBUG ((DEBUG_INFO, "==%a==\n", __func__));

  //
  // Get the the CPU physical memory address width.
  //
  AsmCpucfg (CPUCFG_REG1_INFO, &CpucfgReg1Data.Uint32);

  CpuPhysMemAddressWidth = (UINT8)(CpucfgReg1Data.Bits.PALEN + 1);

  //
  // Create CPU HOBs.
  //
  BuildCpuHob (CpuPhysMemAddressWidth, FixedPcdGet8 (PcdPrePiCpuIoSize));
}

/**
  add fdt hand off block.

  @param  VOID

  @return  VOID
**/
STATIC
VOID
AddFdtHob (
  VOID
  )
{
  VOID           *Base;
  VOID           *NewBase;
  UINTN          FdtSize;
  UINTN          FdtPages;
  UINT64         *FdtHobData;
  UINT64         RtcBaseAddress;
  RETURN_STATUS  Status;

  Base = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  ASSERT (Base != NULL);

  Status = GetRtcAddress (Base, &RtcBaseAddress);
  if (RETURN_ERROR (Status)) {
    return;
  }

  SaveRtcRegisterAddressHob (RtcBaseAddress);
  SetupTpmResources (Base);

  FdtSize  = FdtTotalSize (Base) + PcdGet32 (PcdDeviceTreeAllocationPadding);
  FdtPages = EFI_SIZE_TO_PAGES (FdtSize);
  NewBase  = AllocatePages (FdtPages);
  ASSERT (NewBase != NULL);
  FdtOpenInto (Base, NewBase, EFI_PAGES_TO_SIZE (FdtPages));

  FdtHobData = BuildGuidHob (&gFdtHobGuid, sizeof *FdtHobData);
  ASSERT (FdtHobData != NULL);
  *FdtHobData = (UINTN)NewBase;
}

/**
  Fetch the size of system memory from QEMU.

  @param  VOID

  @return  VOID
**/
STATIC
VOID
ReportSystemMemorySize (
  VOID
  )
{
  UINT64  RamSize;

  QemuFwCfgSelectItem (QemuFwCfgItemRamSize);
  RamSize = QemuFwCfgRead64 ();
  DEBUG ((
    DEBUG_INFO,
    "%a: QEMU reports %dM system memory\n",
    __func__,
    RamSize/1024/1024
    ));
}

/**
  Perform Platform PEI initialization.

  @param  FileHandle      Handle of the file being invoked.
  @param  PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS     The PEIM initialized successfully.
**/
EFI_STATUS
EFIAPI
InitializePlatform (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS             Status;
  EFI_MEMORY_DESCRIPTOR  *MemoryTable;

  DEBUG ((DEBUG_INFO, "Platform PEIM Loaded\n"));

  Status = PeiServicesSetBootMode (mBootMode);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesInstallPpi (&mPpiListBootMode);
  ASSERT_EFI_ERROR (Status);

  ReportSystemMemorySize ();

  PublishPeiMemory ();

  PeiFvInitialization ();
  InitializeRamRegions ();
  MemMapInitialization ();

  Status = PlatformHookSerialPortInitialize ();
  ASSERT_EFI_ERROR (Status);

  MiscInitialization ();

  AddFdtHob ();

  //
  // Initialization MMU
  //
  GetMemoryMapPolicy (&MemoryTable);
  Status = ConfigureMemoryManagementUnit (MemoryTable);
  ASSERT_EFI_ERROR (Status);

  MpInitLibInitialize ();

  return EFI_SUCCESS;
}
