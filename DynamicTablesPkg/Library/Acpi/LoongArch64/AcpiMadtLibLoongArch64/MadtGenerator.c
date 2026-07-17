/** @file
  MADT Table Generator for LoongArch64

  Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.5 Specification, Aug 29, 2022
**/

#include <IndustryStandard/Acpi65.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

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

#define LOONGARCH_PIC_VERSION           1
#define LOONGARCH_CORE_PIC_ENABLED      BIT0
#define LOONGARCH_CORE_PIC_VALID_FLAGS  (BIT0 | BIT1)

/** Validate the LoongArch64 PIC CM objects.

  @param [in] CorePicInfo   Pointer to the Core PIC CM object list.
  @param [in] CorePicCount  Number of Core PIC objects.
  @param [in] LioPicInfo    Pointer to the LIO PIC CM object list.
  @param [in] LioPicCount   Number of LIO PIC objects.
  @param [in] HtPicInfo     Pointer to the HT PIC CM object list.
  @param [in] HtPicCount    Number of HT PIC objects.
  @param [in] EioPicInfo    Pointer to the EIO PIC CM object list.
  @param [in] EioPicCount   Number of EIO PIC objects.
  @param [in] MsiPicInfo    Pointer to the MSI PIC CM object list.
  @param [in] MsiPicCount   Number of MSI PIC objects.
  @param [in] BioPicInfo    Pointer to the BIO PIC CM object list.
  @param [in] BioPicCount   Number of BIO PIC objects.
  @param [in] LpcPicInfo    Pointer to the LPC PIC CM object list.
  @param [in] LpcPicCount   Number of LPC PIC objects.

  @retval EFI_SUCCESS            The PIC objects are valid.
  @retval EFI_INVALID_PARAMETER  A PIC object is invalid.
**/
STATIC
EFI_STATUS
ValidatePicInfo (
  IN CONST CM_LOONGARCH64_CORE_PIC_INFO  *CorePicInfo,
  IN       UINT32                        CorePicCount,
  IN CONST CM_LOONGARCH64_LIO_PIC_INFO   *LioPicInfo,
  IN       UINT32                        LioPicCount,
  IN CONST CM_LOONGARCH64_HT_PIC_INFO    *HtPicInfo,
  IN       UINT32                        HtPicCount,
  IN CONST CM_LOONGARCH64_EIO_PIC_INFO   *EioPicInfo,
  IN       UINT32                        EioPicCount,
  IN CONST CM_LOONGARCH64_MSI_PIC_INFO   *MsiPicInfo,
  IN       UINT32                        MsiPicCount,
  IN CONST CM_LOONGARCH64_BIO_PIC_INFO   *BioPicInfo,
  IN       UINT32                        BioPicCount,
  IN CONST CM_LOONGARCH64_LPC_PIC_INFO   *LpcPicInfo,
  IN       UINT32                        LpcPicCount
  )
{
  BOOLEAN  HasEnabledCore;
  UINT32   Index;
  UINT32   OtherIndex;

  HasEnabledCore = FALSE;
  for (Index = 0; Index < CorePicCount; Index++) {
    if ((CorePicInfo[Index].Version != LOONGARCH_PIC_VERSION) ||
        ((CorePicInfo[Index].Flags & ~LOONGARCH_CORE_PIC_VALID_FLAGS) != 0))
    {
      DEBUG ((DEBUG_ERROR, "ERROR: MADT: Invalid Core PIC object at index %u.\n", Index));
      return EFI_INVALID_PARAMETER;
    }

    HasEnabledCore |= ((CorePicInfo[Index].Flags & LOONGARCH_CORE_PIC_ENABLED) != 0);
    for (OtherIndex = Index + 1; OtherIndex < CorePicCount; OtherIndex++) {
      if ((CorePicInfo[Index].ProcessorId == CorePicInfo[OtherIndex].ProcessorId) ||
          (CorePicInfo[Index].CoreId == CorePicInfo[OtherIndex].CoreId))
      {
        DEBUG ((DEBUG_ERROR, "ERROR: MADT: Duplicate Core PIC identifier.\n"));
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  if (!HasEnabledCore) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: No enabled Core PIC object.\n"));
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < LioPicCount; Index++) {
    if ((LioPicInfo[Index].Version != LOONGARCH_PIC_VERSION) ||
        (LioPicInfo[Index].Size == 0) ||
        (LioPicInfo[Index].Address > (MAX_UINT64 - (LioPicInfo[Index].Size - 1))))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  for (Index = 0; Index < HtPicCount; Index++) {
    if ((HtPicInfo[Index].Version != LOONGARCH_PIC_VERSION) ||
        (HtPicInfo[Index].Size == 0) ||
        (HtPicInfo[Index].Address > (MAX_UINT64 - (HtPicInfo[Index].Size - 1))))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  for (Index = 0; Index < EioPicCount; Index++) {
    if (EioPicInfo[Index].Version != LOONGARCH_PIC_VERSION) {
      return EFI_INVALID_PARAMETER;
    }
  }

  for (Index = 0; Index < MsiPicCount; Index++) {
    if ((MsiPicInfo[Index].Version != LOONGARCH_PIC_VERSION) ||
        (MsiPicInfo[Index].Count == 0) ||
        (MsiPicInfo[Index].Start > (MAX_UINT32 - (MsiPicInfo[Index].Count - 1))))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  for (Index = 0; Index < BioPicCount; Index++) {
    if ((BioPicInfo[Index].Version != LOONGARCH_PIC_VERSION) ||
        (BioPicInfo[Index].Size == 0) ||
        (BioPicInfo[Index].Address > (MAX_UINT64 - (BioPicInfo[Index].Size - 1))))
    {
      return EFI_INVALID_PARAMETER;
    }

    for (OtherIndex = Index + 1; OtherIndex < BioPicCount; OtherIndex++) {
      if (BioPicInfo[Index].Id == BioPicInfo[OtherIndex].Id) {
        DEBUG ((DEBUG_ERROR, "ERROR: MADT: Duplicate BIO PIC identifier.\n"));
        return EFI_INVALID_PARAMETER;
      }
    }
  }

  for (Index = 0; Index < LpcPicCount; Index++) {
    if ((LpcPicInfo[Index].Version != LOONGARCH_PIC_VERSION) ||
        (LpcPicInfo[Index].Size == 0) ||
        (LpcPicInfo[Index].Address > (MAX_UINT64 - (LpcPicInfo[Index].Size - 1))))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/** Add Core PIC structures to the MADT.

  @param [out] CorePic       Pointer to the first output Core PIC structure.
  @param [in]  CorePicInfo   Pointer to the Core PIC CM object list.
  @param [in]  CorePicCount  Number of Core PIC structures to add.
**/
STATIC
VOID
AddCorePicList (
  IN  EFI_ACPI_6_5_CORE_PIC_STRUCTURE     *CorePic,
  IN  CONST CM_LOONGARCH64_CORE_PIC_INFO  *CorePicInfo,
  IN  UINT32                              CorePicCount
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

/** Add LIO PIC structures to the MADT.

  @param [out] LioPic       Pointer to the first output LIO PIC structure.
  @param [in]  LioPicInfo   Pointer to the LIO PIC CM object list.
  @param [in]  LioPicCount  Number of LIO PIC structures to add.
**/
STATIC
VOID
AddLioPicList (
  IN  EFI_ACPI_6_5_LIO_PIC_STRUCTURE     *LioPic,
  IN  CONST CM_LOONGARCH64_LIO_PIC_INFO  *LioPicInfo,
  IN  UINT32                             LioPicCount
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

/** Add HT PIC structures to the MADT.

  @param [out] HtPic       Pointer to the first output HT PIC structure.
  @param [in]  HtPicInfo   Pointer to the HT PIC CM object list.
  @param [in]  HtPicCount  Number of HT PIC structures to add.
**/
STATIC
VOID
AddHtPicList (
  IN  EFI_ACPI_6_5_HT_PIC_STRUCTURE     *HtPic,
  IN  CONST CM_LOONGARCH64_HT_PIC_INFO  *HtPicInfo,
  IN  UINT32                            HtPicCount
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

/** Add EIO PIC structures to the MADT.

  @param [out] EioPic       Pointer to the first output EIO PIC structure.
  @param [in]  EioPicInfo   Pointer to the EIO PIC CM object list.
  @param [in]  EioPicCount  Number of EIO PIC structures to add.
**/
STATIC
VOID
AddEioPicList (
  IN  EFI_ACPI_6_5_EIO_PIC_STRUCTURE     *EioPic,
  IN  CONST CM_LOONGARCH64_EIO_PIC_INFO  *EioPicInfo,
  IN  UINT32                             EioPicCount
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

/** Add MSI PIC structures to the MADT.

  @param [out] MsiPic       Pointer to the first output MSI PIC structure.
  @param [in]  MsiPicInfo   Pointer to the MSI PIC CM object list.
  @param [in]  MsiPicCount  Number of MSI PIC structures to add.
**/
STATIC
VOID
AddMsiPicList (
  IN  EFI_ACPI_6_5_MSI_PIC_STRUCTURE     *MsiPic,
  IN  CONST CM_LOONGARCH64_MSI_PIC_INFO  *MsiPicInfo,
  IN  UINT32                             MsiPicCount
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

/** Add BIO PIC structures to the MADT.

  @param [out] BioPic       Pointer to the first output BIO PIC structure.
  @param [in]  BioPicInfo   Pointer to the BIO PIC CM object list.
  @param [in]  BioPicCount  Number of BIO PIC structures to add.
**/
STATIC
VOID
AddBioPicList (
  IN  EFI_ACPI_6_5_BIO_PIC_STRUCTURE     *BioPic,
  IN  CONST CM_LOONGARCH64_BIO_PIC_INFO  *BioPicInfo,
  IN  UINT32                             BioPicCount
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

/** Add LPC PIC structures to the MADT.

  @param [out] LpcPic       Pointer to the first output LPC PIC structure.
  @param [in]  LpcPicInfo   Pointer to the LPC PIC CM object list.
  @param [in]  LpcPicCount  Number of LPC PIC structures to add.
**/
STATIC
VOID
AddLpcPicList (
  IN  EFI_ACPI_6_5_LPC_PIC_STRUCTURE     *LpcPic,
  IN  CONST CM_LOONGARCH64_LPC_PIC_INFO  *LpcPicInfo,
  IN  UINT32                             LpcPicCount
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

/** Construct the LoongArch64 MADT.

  @param [in]  This            Pointer to the table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager protocol.
  @param [out] Table           Pointer to the constructed ACPI table.

  @retval EFI_SUCCESS            The table was generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          A required CM object was not found.
  @retval EFI_BAD_BUFFER_SIZE    The generated table would exceed MAX_UINT32.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate the table.
**/
STATIC
EFI_STATUS
EFIAPI
BuildMadtTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                                           Status;
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *Madt;
  CM_LOONGARCH64_MADT_INFO                             *MadtInfo;
  CM_LOONGARCH64_CORE_PIC_INFO                         *CorePicInfo;
  CM_LOONGARCH64_LIO_PIC_INFO                          *LioPicInfo;
  CM_LOONGARCH64_HT_PIC_INFO                           *HtPicInfo;
  CM_LOONGARCH64_EIO_PIC_INFO                          *EioPicInfo;
  CM_LOONGARCH64_MSI_PIC_INFO                          *MsiPicInfo;
  CM_LOONGARCH64_BIO_PIC_INFO                          *BioPicInfo;
  CM_LOONGARCH64_LPC_PIC_INFO                          *LpcPicInfo;
  UINT32                                               CorePicCount;
  UINT32                                               LioPicCount;
  UINT32                                               HtPicCount;
  UINT32                                               EioPicCount;
  UINT32                                               MsiPicCount;
  UINT32                                               BioPicCount;
  UINT32                                               LpcPicCount;
  UINT32                                               MadtInfoCount;
  UINT32                                               TableSize;
  UINT64                                               TableSize64;
  UINT64                                               TableOffset64;
  UINT32                                               CorePicOffset;
  UINT32                                               LioPicOffset;
  UINT32                                               HtPicOffset;
  UINT32                                               EioPicOffset;
  UINT32                                               MsiPicOffset;
  UINT32                                               BioPicOffset;
  UINT32                                               LpcPicOffset;

  if ((This == NULL) || (AcpiTableInfo == NULL) ||
      (CfgMgrProtocol == NULL) || (Table == NULL))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Invalid parameter.\n"));
    return EFI_INVALID_PARAMETER;
  }

  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->TableGeneratorId != This->GeneratorID) ||
      (AcpiTableInfo->AcpiTableSignature != This->AcpiTableSignature))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Invalid table generator information.\n"));
    return EFI_INVALID_PARAMETER;
  }

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

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Requested table revision = %d is not supported. "
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  MadtInfoCount = 0;
  Status        = GetELoongArch64ObjMadtInfo (
                    CfgMgrProtocol,
                    CM_NULL_TOKEN,
                    &MadtInfo,
                    &MadtInfoCount
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get MADT Info. Status = %r\n", Status));
    return Status;
  }

  if ((MadtInfo == NULL) || (MadtInfoCount != 1)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Exactly one MADT Info object must be provided. Count = %u\n",
      MadtInfoCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((MadtInfo->Flags & ~(UINT32)EFI_ACPI_6_5_PCAT_COMPAT) != 0) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Invalid MADT flags = 0x%x\n", MadtInfo->Flags));
    return EFI_INVALID_PARAMETER;
  }

  CorePicCount = 0;
  Status       = GetELoongArch64ObjCorePicInfo (
                   CfgMgrProtocol,
                   CM_NULL_TOKEN,
                   &CorePicInfo,
                   &CorePicCount
                   );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get Core PIC Info. Status = %r\n", Status));
    return Status;
  }

  if ((CorePicInfo == NULL) || (CorePicCount == 0)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Core PIC information not provided.\n"));
    return EFI_NOT_FOUND;
  }

  Status = GetELoongArch64ObjLioPicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &LioPicInfo,
             &LioPicCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get LIO PIC Info. Status = %r\n", Status));
    return Status;
  }

  Status = GetELoongArch64ObjHtPicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &HtPicInfo,
             &HtPicCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get HT PIC Info. Status = %r\n", Status));
    return Status;
  }

  Status = GetELoongArch64ObjEioPicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &EioPicInfo,
             &EioPicCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get EIO PIC Info. Status = %r\n", Status));
    return Status;
  }

  Status = GetELoongArch64ObjMsiPicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &MsiPicInfo,
             &MsiPicCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get MSI PIC Info. Status = %r\n", Status));
    return Status;
  }

  Status = GetELoongArch64ObjBioPicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &BioPicInfo,
             &BioPicCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get BIO PIC Info. Status = %r\n", Status));
    return Status;
  }

  Status = GetELoongArch64ObjLpcPicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &LpcPicInfo,
             &LpcPicCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get LPC PIC Info. Status = %r\n", Status));
    return Status;
  }

  if (((LioPicCount != 0) && (LioPicInfo == NULL)) ||
      ((HtPicCount != 0) && (HtPicInfo == NULL)) ||
      ((EioPicCount != 0) && (EioPicInfo == NULL)) ||
      ((MsiPicCount != 0) && (MsiPicInfo == NULL)) ||
      ((BioPicCount != 0) && (BioPicInfo == NULL)) ||
      ((LpcPicCount != 0) && (LpcPicInfo == NULL)))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Invalid PIC object list.\n"));
    return EFI_INVALID_PARAMETER;
  }

  TableSize64  = sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);
  TableSize64 += (UINT64)sizeof (EFI_ACPI_6_5_CORE_PIC_STRUCTURE) * CorePicCount;
  TableSize64 += (UINT64)sizeof (EFI_ACPI_6_5_LIO_PIC_STRUCTURE) * LioPicCount;
  TableSize64 += (UINT64)sizeof (EFI_ACPI_6_5_HT_PIC_STRUCTURE) * HtPicCount;
  TableSize64 += (UINT64)sizeof (EFI_ACPI_6_5_EIO_PIC_STRUCTURE) * EioPicCount;
  TableSize64 += (UINT64)sizeof (EFI_ACPI_6_5_MSI_PIC_STRUCTURE) * MsiPicCount;
  TableSize64 += (UINT64)sizeof (EFI_ACPI_6_5_BIO_PIC_STRUCTURE) * BioPicCount;
  TableSize64 += (UINT64)sizeof (EFI_ACPI_6_5_LPC_PIC_STRUCTURE) * LpcPicCount;

  if (TableSize64 > MAX_UINT32) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Table size exceeds MAX_UINT32.\n"));
    return EFI_BAD_BUFFER_SIZE;
  }

  TableSize     = (UINT32)TableSize64;
  TableOffset64 = sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);

  CorePicOffset  = (UINT32)TableOffset64;
  TableOffset64 += (UINT64)sizeof (EFI_ACPI_6_5_CORE_PIC_STRUCTURE) * CorePicCount;
  LioPicOffset   = (UINT32)TableOffset64;
  TableOffset64 += (UINT64)sizeof (EFI_ACPI_6_5_LIO_PIC_STRUCTURE) * LioPicCount;
  HtPicOffset    = (UINT32)TableOffset64;
  TableOffset64 += (UINT64)sizeof (EFI_ACPI_6_5_HT_PIC_STRUCTURE) * HtPicCount;
  EioPicOffset   = (UINT32)TableOffset64;
  TableOffset64 += (UINT64)sizeof (EFI_ACPI_6_5_EIO_PIC_STRUCTURE) * EioPicCount;
  MsiPicOffset   = (UINT32)TableOffset64;
  TableOffset64 += (UINT64)sizeof (EFI_ACPI_6_5_MSI_PIC_STRUCTURE) * MsiPicCount;
  BioPicOffset   = (UINT32)TableOffset64;
  TableOffset64 += (UINT64)sizeof (EFI_ACPI_6_5_BIO_PIC_STRUCTURE) * BioPicCount;
  LpcPicOffset   = (UINT32)TableOffset64;

  Status = ValidatePicInfo (
             CorePicInfo,
             CorePicCount,
             LioPicInfo,
             LioPicCount,
             HtPicInfo,
             HtPicCount,
             EioPicInfo,
             EioPicCount,
             MsiPicInfo,
             MsiPicCount,
             BioPicInfo,
             BioPicCount,
             LpcPicInfo,
             LpcPicCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Invalid PIC information. Status = %r\n", Status));
    return Status;
  }

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

  Madt->LocalApicAddress = MadtInfo->LocalApicAddress;
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

/** Free resources allocated for the MADT.

  @param [in]      This            Pointer to the table generator.
  @param [in]      AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]      CfgMgrProtocol  Pointer to the Configuration Manager protocol.
  @param [in, out] Table           Pointer to the ACPI table to free.

  @retval EFI_SUCCESS            The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER  The table pointer is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeMadtTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  if ((This == NULL) || (AcpiTableInfo == NULL) ||
      (CfgMgrProtocol == NULL) || (Table == NULL) || (*Table == NULL))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Invalid parameter.\n"));
    return EFI_INVALID_PARAMETER;
  }

  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->TableGeneratorId != This->GeneratorID) ||
      (AcpiTableInfo->AcpiTableSignature != This->AcpiTableSignature))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Invalid table generator information.\n"));
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

/** Register the LoongArch64 MADT generator.

  @param [in] ImageHandle  The image handle.
  @param [in] SystemTable  Pointer to the system table.

  @retval EFI_SUCCESS  The generator was registered successfully.
  @retval Others       Failed to register the generator.
**/
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

/** Deregister the LoongArch64 MADT generator.

  @param [in] ImageHandle  The image handle.
  @param [in] SystemTable  Pointer to the system table.

  @retval EFI_SUCCESS  The generator was deregistered successfully.
  @retval Others       Failed to deregister the generator.
**/
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
