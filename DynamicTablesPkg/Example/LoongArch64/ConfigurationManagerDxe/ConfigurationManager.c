/** @file

  LoongArch64 Configuration Manager example for native MADT generation.

  Core PIC objects are discovered from EFI_MP_SERVICES_PROTOCOL. Platform
  interrupt-controller topology is provided separately by PlatformMadtData.c.

  Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#include <IndustryStandard/Acpi65.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/MpService.h>

#include "ConfigurationManager.h"

#define EXAMPLE_CONFIGURATION_MANAGER_REVISION  CREATE_REVISION (1, 0)
#define LOONGARCH64_PIC_VERSION                 1
#define LOONGARCH64_CORE_PIC_ENABLED            BIT0

STATIC CM_STD_OBJ_CONFIGURATION_MANAGER_INFO  mConfigurationManagerInfo = {
  EXAMPLE_CONFIGURATION_MANAGER_REVISION,
  {
    'E',
    'X',
    'A',
    'M',
    'P',
    'L'
  }
};

STATIC CM_STD_OBJ_ACPI_TABLE_INFO  mAcpiTableList[] = {
  {
    EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
    EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
    CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMadt),
    NULL,
    0,
    0,
    0
  }
};

STATIC LOONGARCH64_MADT_PLATFORM_REPOSITORY  *mPlatformRepository;
STATIC CM_LOONGARCH64_CORE_PIC_INFO          *mCorePicInfo;
STATIC UINT32                                mCorePicCount;

/** Validate an optional platform object list. **/
STATIC
EFI_STATUS
ValidateOptionalCmObjectList (
  IN CONST VOID    *Data,
  IN CONST UINT32  ObjectSize,
  IN CONST UINT32  Count
  )
{
  if ((Data == NULL) != (Count == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((ObjectSize == 0) || (Count > (MAX_UINT32 / ObjectSize))) {
    return EFI_BAD_BUFFER_SIZE;
  }

  return EFI_SUCCESS;
}

/** Validate the platform-owned MADT repository contract. **/
STATIC
EFI_STATUS
ValidatePlatformRepository (
  IN CONST LOONGARCH64_MADT_PLATFORM_REPOSITORY  *Repository
  )
{
  EFI_STATUS  Status;

  if ((Repository == NULL) || (Repository->MapProcessor == NULL) ||
      (Repository->MaximumProcessorCount == 0) ||
      ((Repository->MadtInfo.Flags & ~EFI_ACPI_6_5_PCAT_COMPAT) != 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = ValidateOptionalCmObjectList (
             Repository->LioPicInfo,
             sizeof (Repository->LioPicInfo[0]),
             Repository->LioPicCount
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateOptionalCmObjectList (
             Repository->HtPicInfo,
             sizeof (Repository->HtPicInfo[0]),
             Repository->HtPicCount
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateOptionalCmObjectList (
             Repository->EioPicInfo,
             sizeof (Repository->EioPicInfo[0]),
             Repository->EioPicCount
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateOptionalCmObjectList (
             Repository->MsiPicInfo,
             sizeof (Repository->MsiPicInfo[0]),
             Repository->MsiPicCount
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ValidateOptionalCmObjectList (
             Repository->BioPicInfo,
             sizeof (Repository->BioPicInfo[0]),
             Repository->BioPicCount
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return ValidateOptionalCmObjectList (
           Repository->LpcPicInfo,
           sizeof (Repository->LpcPicInfo[0]),
           Repository->LpcPicCount
           );
}

/** Return one or more Configuration Manager objects. **/
STATIC
EFI_STATUS
HandleCmObject (
  IN  CONST CM_OBJECT_ID       CmObjectId,
  IN        VOID               *Data,
  IN  CONST UINT32             ObjectSize,
  IN  CONST UINT32             Count,
  OUT       CM_OBJ_DESCRIPTOR  *CmObject
  )
{
  if ((Data == NULL) || (ObjectSize == 0) || (Count == 0) ||
      (CmObject == NULL) || (Count > (MAX_UINT32 / ObjectSize)))
  {
    return EFI_INVALID_PARAMETER;
  }

  CmObject->ObjectId = CmObjectId;
  CmObject->Size     = ObjectSize * Count;
  CmObject->Data     = Data;
  CmObject->Count    = Count;

  return EFI_SUCCESS;
}

/** Return an optional Configuration Manager object list. **/
STATIC
EFI_STATUS
HandleOptionalCmObject (
  IN  CONST CM_OBJECT_ID       CmObjectId,
  IN        VOID               *Data,
  IN  CONST UINT32             ObjectSize,
  IN  CONST UINT32             Count,
  OUT       CM_OBJ_DESCRIPTOR  *CmObject
  )
{
  if (Count == 0) {
    return EFI_NOT_FOUND;
  }

  return HandleCmObject (
           CmObjectId,
           Data,
           ObjectSize,
           Count,
           CmObject
           );
}

/** Build Core PIC objects from the processors exposed by MP Services. **/
STATIC
EFI_STATUS
BuildCorePicInfo (
  VOID
  )
{
  EFI_MP_SERVICES_PROTOCOL   *MpServices;
  EFI_PROCESSOR_INFORMATION  ProcessorInfo;
  EFI_STATUS                 Status;
  UINTN                      NumberOfProcessors;
  UINTN                      NumberOfEnabledProcessors;
  UINTN                      ProcessorIndex;
  BOOLEAN                    Publish;
  UINT32                     CoreId;
  UINT32                     EnabledPicCount;
  UINT32                     Flags;
  UINT32                     ProcessorUid;
  UINT32                     PublishedIndex;

  Status = gBS->LocateProtocol (
                  &gEfiMpServiceProtocolGuid,
                  NULL,
                  (VOID **)&MpServices
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = MpServices->GetNumberOfProcessors (
                         MpServices,
                         &NumberOfProcessors,
                         &NumberOfEnabledProcessors
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((NumberOfProcessors == 0) ||
      (NumberOfProcessors > (MAX_UINT32 / sizeof (*mCorePicInfo))))
  {
    return EFI_BAD_BUFFER_SIZE;
  }

  mCorePicInfo = AllocateZeroPool (
                   NumberOfProcessors * sizeof (*mCorePicInfo)
                   );
  if (mCorePicInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EnabledPicCount = 0;
  PublishedIndex  = 0;
  for (ProcessorIndex = 0;
       ProcessorIndex < NumberOfProcessors;
       ProcessorIndex++)
  {
    Status = MpServices->GetProcessorInfo (
                           MpServices,
                           ProcessorIndex,
                           &ProcessorInfo
                           );
    if (EFI_ERROR (Status)) {
      goto ErrorHandler;
    }

    Status = mPlatformRepository->MapProcessor (
                                    ProcessorIndex,
                                    PublishedIndex,
                                    &ProcessorInfo,
                                    &Publish,
                                    &ProcessorUid,
                                    &CoreId,
                                    &Flags
                                    );
    if (EFI_ERROR (Status)) {
      goto ErrorHandler;
    }

    if (!Publish) {
      continue;
    }

    if ((PublishedIndex >= mPlatformRepository->MaximumProcessorCount) ||
        ((Flags & ~(BIT0 | BIT1)) != 0))
    {
      Status = EFI_INVALID_PARAMETER;
      goto ErrorHandler;
    }

    mCorePicInfo[PublishedIndex].Version     = LOONGARCH64_PIC_VERSION;
    mCorePicInfo[PublishedIndex].ProcessorId = ProcessorUid;
    mCorePicInfo[PublishedIndex].CoreId      = CoreId;
    mCorePicInfo[PublishedIndex].Flags       = Flags;
    if ((Flags & LOONGARCH64_CORE_PIC_ENABLED) != 0) {
      EnabledPicCount++;
    }

    PublishedIndex++;
  }

  if ((PublishedIndex == 0) || (EnabledPicCount == 0)) {
    Status = EFI_NOT_FOUND;
    goto ErrorHandler;
  }

  mCorePicCount = PublishedIndex;
  DEBUG ((
    DEBUG_INFO,
    "LoongArch64 MADT CM: published %u of %Lu processors (%Lu enabled)\n",
    mCorePicCount,
    (UINT64)NumberOfProcessors,
    (UINT64)NumberOfEnabledProcessors
    ));
  return EFI_SUCCESS;

ErrorHandler:
  FreePool (mCorePicInfo);
  mCorePicInfo = NULL;
  return Status;
}

/** Return a Standard namespace object. **/
STATIC
EFI_STATUS
GetStandardNameSpaceObject (
  IN  CONST CM_OBJECT_ID       CmObjectId,
  IN  CONST CM_OBJECT_TOKEN    Token,
  OUT       CM_OBJ_DESCRIPTOR  *CmObject
  )
{
  if (Token != CM_NULL_TOKEN) {
    return EFI_NOT_FOUND;
  }

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    case EStdObjCfgMgrInfo:
      return HandleCmObject (
               CmObjectId,
               &mConfigurationManagerInfo,
               sizeof (mConfigurationManagerInfo),
               1,
               CmObject
               );

    case EStdObjAcpiTableList:
      return HandleCmObject (
               CmObjectId,
               mAcpiTableList,
               sizeof (mAcpiTableList[0]),
               ARRAY_SIZE (mAcpiTableList),
               CmObject
               );

    default:
      return EFI_NOT_FOUND;
  }
}

/** Return a LoongArch64 namespace object. **/
STATIC
EFI_STATUS
GetLoongArch64NameSpaceObject (
  IN  CONST CM_OBJECT_ID       CmObjectId,
  IN  CONST CM_OBJECT_TOKEN    Token,
  OUT       CM_OBJ_DESCRIPTOR  *CmObject
  )
{
  if (Token != CM_NULL_TOKEN) {
    return EFI_NOT_FOUND;
  }

  switch (GET_CM_OBJECT_ID (CmObjectId)) {
    case ELoongArch64ObjMadtInfo:
      return HandleCmObject (
               CmObjectId,
               &mPlatformRepository->MadtInfo,
               sizeof (mPlatformRepository->MadtInfo),
               1,
               CmObject
               );

    case ELoongArch64ObjCorePicInfo:
      return HandleCmObject (
               CmObjectId,
               mCorePicInfo,
               sizeof (mCorePicInfo[0]),
               mCorePicCount,
               CmObject
               );

    case ELoongArch64ObjLioPicInfo:
      return HandleOptionalCmObject (
               CmObjectId,
               mPlatformRepository->LioPicInfo,
               sizeof (mPlatformRepository->LioPicInfo[0]),
               mPlatformRepository->LioPicCount,
               CmObject
               );

    case ELoongArch64ObjHtPicInfo:
      return HandleOptionalCmObject (
               CmObjectId,
               mPlatformRepository->HtPicInfo,
               sizeof (mPlatformRepository->HtPicInfo[0]),
               mPlatformRepository->HtPicCount,
               CmObject
               );

    case ELoongArch64ObjEioPicInfo:
      return HandleOptionalCmObject (
               CmObjectId,
               mPlatformRepository->EioPicInfo,
               sizeof (mPlatformRepository->EioPicInfo[0]),
               mPlatformRepository->EioPicCount,
               CmObject
               );

    case ELoongArch64ObjMsiPicInfo:
      return HandleOptionalCmObject (
               CmObjectId,
               mPlatformRepository->MsiPicInfo,
               sizeof (mPlatformRepository->MsiPicInfo[0]),
               mPlatformRepository->MsiPicCount,
               CmObject
               );

    case ELoongArch64ObjBioPicInfo:
      return HandleOptionalCmObject (
               CmObjectId,
               mPlatformRepository->BioPicInfo,
               sizeof (mPlatformRepository->BioPicInfo[0]),
               mPlatformRepository->BioPicCount,
               CmObject
               );

    case ELoongArch64ObjLpcPicInfo:
      return HandleOptionalCmObject (
               CmObjectId,
               mPlatformRepository->LpcPicInfo,
               sizeof (mPlatformRepository->LpcPicInfo[0]),
               mPlatformRepository->LpcPicCount,
               CmObject
               );

    default:
      return EFI_NOT_FOUND;
  }
}

/** Return an object from the platform Configuration Manager repository. **/
STATIC
EFI_STATUS
EFIAPI
PlatformGetObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  if ((This == NULL) || (CmObject == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (GET_CM_NAMESPACE_ID (CmObjectId)) {
    case EObjNameSpaceStandard:
      return GetStandardNameSpaceObject (CmObjectId, Token, CmObject);

    case EObjNameSpaceLoongArch64:
      return GetLoongArch64NameSpaceObject (CmObjectId, Token, CmObject);

    default:
      return EFI_INVALID_PARAMETER;
  }
}

/** Updating Configuration Manager objects is not supported. **/
STATIC
EFI_STATUS
EFIAPI
PlatformSetObject (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token OPTIONAL,
  IN        CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  )
{
  return EFI_UNSUPPORTED;
}

STATIC EDKII_CONFIGURATION_MANAGER_PROTOCOL  mConfigurationManagerProtocol = {
  EDKII_CONFIGURATION_MANAGER_PROTOCOL_REVISION,
  PlatformGetObject,
  PlatformSetObject,
  NULL
};

/** Build the repository and install the Configuration Manager protocol. **/
EFI_STATUS
EFIAPI
LoongArch64MadtConfigurationManagerDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mPlatformRepository = GetLoongArch64MadtPlatformRepository ();
  Status              = ValidatePlatformRepository (mPlatformRepository);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = BuildCorePicInfo ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEdkiiConfigurationManagerProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mConfigurationManagerProtocol
                  );
  if (EFI_ERROR (Status)) {
    FreePool (mCorePicInfo);
    mCorePicInfo  = NULL;
    mCorePicCount = 0;
  }

  return Status;
}
