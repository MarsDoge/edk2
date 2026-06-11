/** @file
  MADT Table Generator for LoongArch64.

  Copyright (c) 2026, MarsDoge. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi.h>
#include <Library/AcpiHelperLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

GET_OBJECT_LIST (
  EObjNameSpaceLoongArch,
  ELoongArchObjCorePicInfo,
  CM_LOONGARCH_CORE_PIC_INFO
  );

STATIC
VOID
AddCorePic (
  IN OUT EFI_ACPI_6_5_CORE_PIC_STRUCTURE       *CorePic,
  IN     CONST CM_LOONGARCH_CORE_PIC_INFO      *CorePicInfo
  )
{
  CorePic->Type        = EFI_ACPI_6_5_CORE_PIC;
  CorePic->Length      = sizeof (EFI_ACPI_6_5_CORE_PIC_STRUCTURE);
  CorePic->Version     = CorePicInfo->Version;
  CorePic->ProcessorId = CorePicInfo->ProcessorId;
  CorePic->CoreId      = CorePicInfo->CoreId;
  CorePic->Flags       = CorePicInfo->Flags;
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
  EFI_STATUS                                      Status;
  CM_LOONGARCH_CORE_PIC_INFO                      *CorePicInfo;
  UINT32                                          CorePicCount;
  UINT32                                          Index;
  UINT32                                          TableSize;
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *Madt;
  EFI_ACPI_6_5_CORE_PIC_STRUCTURE                 *CorePic;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);

  *Table = NULL;

  Status = GetELoongArchObjCorePicInfo (CfgMgrProtocol, CM_NULL_TOKEN, &CorePicInfo, &CorePicCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get LoongArch Core PIC info. Status = %r\n", Status));
    return Status;
  }

  TableSize = sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER) +
              (CorePicCount * sizeof (EFI_ACPI_6_5_CORE_PIC_STRUCTURE));

  Madt = AllocateZeroPool (TableSize);
  if (Madt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Madt->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    FreePool (Madt);
    return Status;
  }

  CorePic = (EFI_ACPI_6_5_CORE_PIC_STRUCTURE *)(Madt + 1);
  for (Index = 0; Index < CorePicCount; Index++) {
    AddCorePic (&CorePic[Index], &CorePicInfo[Index]);
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)Madt;
  return EFI_SUCCESS;
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
  if ((Table == NULL) || (*Table == NULL)) {
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
  DEBUG ((DEBUG_INFO, "MADT: Register LoongArch64 Generator. Status = %r\n", Status));
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
  DEBUG ((DEBUG_INFO, "MADT: Deregister LoongArch64 Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
