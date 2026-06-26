/** @file
  MADT Table Generator for LoongArch64

  Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.5 Specification, Aug 29, 2022
**/

#include <IndustryStandard/Acpi65.h>
#include <Library/AcpiHelperLib.h>
#include <Library/AcpiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** LoongArch64 standard MADT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - ELoongArch64ObjMadtInfo
  - ELoongArch64ObjCorePicInfo

  The following Configuration Manager Object(s) are optional:
  - ELoongArch64ObjLioPicInfo
  - ELoongArch64ObjHtPicInfo
  - ELoongArch64ObjEioPicInfo
  - ELoongArch64ObjMsiPicInfo
  - ELoongArch64ObjBioPicInfo
  - ELoongArch64ObjLpcPicInfo
*/

GET_OBJECT_LIST (
  EObjNameSpaceLoongArch64,
  ELoongArch64ObjMadtInfo,
  CM_LOONGARCH64_MADT_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceLoongArch64,
  ELoongArch64ObjCorePicInfo,
  CM_LOONGARCH64_CORE_PIC_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceLoongArch64,
  ELoongArch64ObjLioPicInfo,
  CM_LOONGARCH64_LIO_PIC_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceLoongArch64,
  ELoongArch64ObjHtPicInfo,
  CM_LOONGARCH64_HT_PIC_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceLoongArch64,
  ELoongArch64ObjEioPicInfo,
  CM_LOONGARCH64_EIO_PIC_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceLoongArch64,
  ELoongArch64ObjMsiPicInfo,
  CM_LOONGARCH64_MSI_PIC_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceLoongArch64,
  ELoongArch64ObjBioPicInfo,
  CM_LOONGARCH64_BIO_PIC_INFO
  );

GET_OBJECT_LIST (
  EObjNameSpaceLoongArch64,
  ELoongArch64ObjLpcPicInfo,
  CM_LOONGARCH64_LPC_PIC_INFO
  );

STATIC
VOID
AddCorePicList (
  IN  EFI_ACPI_6_5_CORE_PIC_STRUCTURE        *CorePic,
  IN  CONST CM_LOONGARCH64_CORE_PIC_INFO     *CorePicInfo,
  IN  UINT32                                 CorePicCount
  )
{
  ASSERT (CorePic != NULL);
  ASSERT (CorePicInfo != NULL);

  while (CorePicCount-- != 0) {
    CorePic->Type        = EFI_ACPI_6_5_CORE_PIC;
    CorePic->Length      = sizeof (EFI_ACPI_6_5_CORE_PIC_STRUCTURE);
    CorePic->Version     = CorePicInfo->Version;
    CorePic->ProcessorId = CorePicInfo->ProcessorId;
    CorePic->CoreId      = CorePicInfo->CoreId;
    CorePic->Flags       = CorePicInfo->Flags;

    CorePic++;
    CorePicInfo++;
  }
}

STATIC
VOID
AddLioPicList (
  IN  EFI_ACPI_6_5_LIO_PIC_STRUCTURE        *LioPic,
  IN  CONST CM_LOONGARCH64_LIO_PIC_INFO     *LioPicInfo,
  IN  UINT32                                LioPicCount
  )
{
  ASSERT (LioPic != NULL);
  ASSERT (LioPicInfo != NULL);

  while (LioPicCount-- != 0) {
    LioPic->Type          = EFI_ACPI_6_5_LIO_PIC;
    LioPic->Length        = sizeof (EFI_ACPI_6_5_LIO_PIC_STRUCTURE);
    LioPic->Version       = LioPicInfo->Version;
    LioPic->Address       = LioPicInfo->Address;
    LioPic->Size          = LioPicInfo->Size;
    LioPic->Cascade[0]    = LioPicInfo->Cascade[0];
    LioPic->Cascade[1]    = LioPicInfo->Cascade[1];
    LioPic->CascadeMap[0] = LioPicInfo->CascadeMap[0];
    LioPic->CascadeMap[1] = LioPicInfo->CascadeMap[1];

    LioPic++;
    LioPicInfo++;
  }
}

STATIC
VOID
AddHtPicList (
  IN  EFI_ACPI_6_5_HT_PIC_STRUCTURE        *HtPic,
  IN  CONST CM_LOONGARCH64_HT_PIC_INFO     *HtPicInfo,
  IN  UINT32                               HtPicCount
  )
{
  ASSERT (HtPic != NULL);
  ASSERT (HtPicInfo != NULL);

  while (HtPicCount-- != 0) {
    HtPic->Type    = EFI_ACPI_6_5_HT_PIC;
    HtPic->Length  = sizeof (EFI_ACPI_6_5_HT_PIC_STRUCTURE);
    HtPic->Version = HtPicInfo->Version;
    HtPic->Address = HtPicInfo->Address;
    HtPic->Size    = HtPicInfo->Size;
    CopyMem (HtPic->Cascade, HtPicInfo->Cascade, sizeof (HtPic->Cascade));

    HtPic++;
    HtPicInfo++;
  }
}

STATIC
VOID
AddEioPicList (
  IN  EFI_ACPI_6_5_EIO_PIC_STRUCTURE        *EioPic,
  IN  CONST CM_LOONGARCH64_EIO_PIC_INFO     *EioPicInfo,
  IN  UINT32                                EioPicCount
  )
{
  ASSERT (EioPic != NULL);
  ASSERT (EioPicInfo != NULL);

  while (EioPicCount-- != 0) {
    EioPic->Type    = EFI_ACPI_6_5_EIO_PIC;
    EioPic->Length  = sizeof (EFI_ACPI_6_5_EIO_PIC_STRUCTURE);
    EioPic->Version = EioPicInfo->Version;
    EioPic->Cascade = EioPicInfo->Cascade;
    EioPic->Node    = EioPicInfo->Node;
    EioPic->NodeMap = EioPicInfo->NodeMap;

    EioPic++;
    EioPicInfo++;
  }
}

STATIC
VOID
AddMsiPicList (
  IN  EFI_ACPI_6_5_MSI_PIC_STRUCTURE        *MsiPic,
  IN  CONST CM_LOONGARCH64_MSI_PIC_INFO     *MsiPicInfo,
  IN  UINT32                                MsiPicCount
  )
{
  ASSERT (MsiPic != NULL);
  ASSERT (MsiPicInfo != NULL);

  while (MsiPicCount-- != 0) {
    MsiPic->Type       = EFI_ACPI_6_5_MSI_PIC;
    MsiPic->Length     = sizeof (EFI_ACPI_6_5_MSI_PIC_STRUCTURE);
    MsiPic->Version    = MsiPicInfo->Version;
    MsiPic->MsgAddress = MsiPicInfo->MsgAddress;
    MsiPic->Start      = MsiPicInfo->Start;
    MsiPic->Count      = MsiPicInfo->Count;

    MsiPic++;
    MsiPicInfo++;
  }
}

STATIC
VOID
AddBioPicList (
  IN  EFI_ACPI_6_5_BIO_PIC_STRUCTURE        *BioPic,
  IN  CONST CM_LOONGARCH64_BIO_PIC_INFO     *BioPicInfo,
  IN  UINT32                                BioPicCount
  )
{
  ASSERT (BioPic != NULL);
  ASSERT (BioPicInfo != NULL);

  while (BioPicCount-- != 0) {
    BioPic->Type    = EFI_ACPI_6_5_BIO_PIC;
    BioPic->Length  = sizeof (EFI_ACPI_6_5_BIO_PIC_STRUCTURE);
    BioPic->Version = BioPicInfo->Version;
    BioPic->Address = BioPicInfo->Address;
    BioPic->Size    = BioPicInfo->Size;
    BioPic->Id      = BioPicInfo->Id;
    BioPic->GsiBase = BioPicInfo->GsiBase;

    BioPic++;
    BioPicInfo++;
  }
}

STATIC
VOID
AddLpcPicList (
  IN  EFI_ACPI_6_5_LPC_PIC_STRUCTURE        *LpcPic,
  IN  CONST CM_LOONGARCH64_LPC_PIC_INFO     *LpcPicInfo,
  IN  UINT32                                LpcPicCount
  )
{
  ASSERT (LpcPic != NULL);
  ASSERT (LpcPicInfo != NULL);

  while (LpcPicCount-- != 0) {
    LpcPic->Type    = EFI_ACPI_6_5_LPC_PIC;
    LpcPic->Length  = sizeof (EFI_ACPI_6_5_LPC_PIC_STRUCTURE);
    LpcPic->Version = LpcPicInfo->Version;
    LpcPic->Address = LpcPicInfo->Address;
    LpcPic->Size    = LpcPicInfo->Size;
    LpcPic->Cascade = LpcPicInfo->Cascade;

    LpcPic++;
    LpcPicInfo++;
  }
}

STATIC
EFI_STATUS
BuildMadtTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                                            Status;
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *Madt;
  CM_LOONGARCH64_MADT_INFO                             *MadtInfo;
  CM_LOONGARCH64_CORE_PIC_INFO                         *CorePicInfo;
  CM_LOONGARCH64_LIO_PIC_INFO                     *LioPicInfo;
  CM_LOONGARCH64_HT_PIC_INFO                      *HtPicInfo;
  CM_LOONGARCH64_EIO_PIC_INFO                     *EioPicInfo;
  CM_LOONGARCH64_MSI_PIC_INFO                     *MsiPicInfo;
  CM_LOONGARCH64_BIO_PIC_INFO                     *BioPicInfo;
  CM_LOONGARCH64_LPC_PIC_INFO                     *LpcPicInfo;
  UINT32                                          CorePicCount;
  UINT32                                          LioPicCount;
  UINT32                                          HtPicCount;
  UINT32                                          EioPicCount;
  UINT32                                          MsiPicCount;
  UINT32                                          BioPicCount;
  UINT32                                          LpcPicCount;
  UINT32                                          TableSize;
  UINT32                                          CorePicOffset;
  UINT32                                          LioPicOffset;
  UINT32                                          HtPicOffset;
  UINT32                                          EioPicOffset;
  UINT32                                          MsiPicOffset;
  UINT32                                          BioPicOffset;
  UINT32                                          LpcPicOffset;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  LioPicInfo  = NULL;
  HtPicInfo   = NULL;
  EioPicInfo  = NULL;
  MsiPicInfo  = NULL;
  BioPicInfo  = NULL;
  LpcPicInfo  = NULL;
  LioPicCount = 0;
  HtPicCount  = 0;
  EioPicCount = 0;
  MsiPicCount = 0;
  BioPicCount = 0;
  LpcPicCount = 0;

  if ((This == NULL) || (AcpiTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) ||
      (AcpiTableInfo->TableGeneratorId != This->GeneratorID) ||
      (AcpiTableInfo->AcpiTableSignature != This->AcpiTableSignature))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Invalid parameter.\n"));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = GetELoongArch64ObjMadtInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &MadtInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get MADT Info. Status = %r\n", Status));
    return Status;
  }

  Status = GetELoongArch64ObjCorePicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &CorePicInfo,
             &CorePicCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get Core PIC Info. Status = %r\n", Status));
    return Status;
  }

  if (CorePicCount == 0) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Core PIC Count = 0\n"));
    return EFI_INVALID_PARAMETER;
  }

  Status = GetELoongArch64ObjLioPicInfo (CfgMgrProtocol, CM_NULL_TOKEN, &LioPicInfo, &LioPicCount);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get LIO PIC Info. Status = %r\n", Status));
    return Status;
  }

  Status = GetELoongArch64ObjHtPicInfo (CfgMgrProtocol, CM_NULL_TOKEN, &HtPicInfo, &HtPicCount);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get HT PIC Info. Status = %r\n", Status));
    return Status;
  }

  Status = GetELoongArch64ObjEioPicInfo (CfgMgrProtocol, CM_NULL_TOKEN, &EioPicInfo, &EioPicCount);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get EIO PIC Info. Status = %r\n", Status));
    return Status;
  }

  Status = GetELoongArch64ObjMsiPicInfo (CfgMgrProtocol, CM_NULL_TOKEN, &MsiPicInfo, &MsiPicCount);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get MSI PIC Info. Status = %r\n", Status));
    return Status;
  }

  Status = GetELoongArch64ObjBioPicInfo (CfgMgrProtocol, CM_NULL_TOKEN, &BioPicInfo, &BioPicCount);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get BIO PIC Info. Status = %r\n", Status));
    return Status;
  }

  Status = GetELoongArch64ObjLpcPicInfo (CfgMgrProtocol, CM_NULL_TOKEN, &LpcPicInfo, &LpcPicCount);
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get LPC PIC Info. Status = %r\n", Status));
    return Status;
  }

  TableSize = sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);

  CorePicOffset = TableSize;
  TableSize    += sizeof (EFI_ACPI_6_5_CORE_PIC_STRUCTURE) * CorePicCount;

  LioPicOffset = TableSize;
  TableSize   += sizeof (EFI_ACPI_6_5_LIO_PIC_STRUCTURE) * LioPicCount;

  HtPicOffset = TableSize;
  TableSize  += sizeof (EFI_ACPI_6_5_HT_PIC_STRUCTURE) * HtPicCount;

  EioPicOffset = TableSize;
  TableSize   += sizeof (EFI_ACPI_6_5_EIO_PIC_STRUCTURE) * EioPicCount;

  MsiPicOffset = TableSize;
  TableSize   += sizeof (EFI_ACPI_6_5_MSI_PIC_STRUCTURE) * MsiPicCount;

  BioPicOffset = TableSize;
  TableSize   += sizeof (EFI_ACPI_6_5_BIO_PIC_STRUCTURE) * BioPicCount;

  LpcPicOffset = TableSize;
  TableSize   += sizeof (EFI_ACPI_6_5_LPC_PIC_STRUCTURE) * LpcPicCount;

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to allocate table, Size = %d\n", TableSize));
    return EFI_OUT_OF_RESOURCES;
  }

  Madt = (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)*Table;

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Madt->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to add ACPI header. Status = %r\n", Status));
    goto error_handler;
  }

  Madt->LocalApicAddress = MadtInfo->LocalInterruptControllerAddress;
  Madt->Flags            = MadtInfo->Flags;

  AddCorePicList (
    (EFI_ACPI_6_5_CORE_PIC_STRUCTURE *)((UINT8 *)Madt + CorePicOffset),
    CorePicInfo,
    CorePicCount
    );

  if (LioPicCount != 0) {
    AddLioPicList ((EFI_ACPI_6_5_LIO_PIC_STRUCTURE *)((UINT8 *)Madt + LioPicOffset), LioPicInfo, LioPicCount);
  }

  if (HtPicCount != 0) {
    AddHtPicList ((EFI_ACPI_6_5_HT_PIC_STRUCTURE *)((UINT8 *)Madt + HtPicOffset), HtPicInfo, HtPicCount);
  }

  if (EioPicCount != 0) {
    AddEioPicList ((EFI_ACPI_6_5_EIO_PIC_STRUCTURE *)((UINT8 *)Madt + EioPicOffset), EioPicInfo, EioPicCount);
  }

  if (MsiPicCount != 0) {
    AddMsiPicList ((EFI_ACPI_6_5_MSI_PIC_STRUCTURE *)((UINT8 *)Madt + MsiPicOffset), MsiPicInfo, MsiPicCount);
  }

  if (BioPicCount != 0) {
    AddBioPicList ((EFI_ACPI_6_5_BIO_PIC_STRUCTURE *)((UINT8 *)Madt + BioPicOffset), BioPicInfo, BioPicCount);
  }

  if (LpcPicCount != 0) {
    AddLpcPicList ((EFI_ACPI_6_5_LPC_PIC_STRUCTURE *)((UINT8 *)Madt + LpcPicOffset), LpcPicInfo, LpcPicCount);
  }

  return EFI_SUCCESS;

error_handler:
  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }

  return Status;
}

STATIC
EFI_STATUS
FreeMadtTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

#define MADT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

STATIC
CONST
ACPI_TABLE_GENERATOR  MadtGenerator = {
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMadt),
  L"ACPI.STD.MADT.GENERATOR",
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
  TABLE_GENERATOR_CREATOR_ID,
  MADT_GENERATOR_REVISION,
  BuildMadtTable,
  FreeMadtTableResources,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
AcpiMadtLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&MadtGenerator);
  DEBUG ((DEBUG_INFO, "MADT: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

EFI_STATUS
EFIAPI
AcpiMadtLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&MadtGenerator);
  DEBUG ((DEBUG_INFO, "MADT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
