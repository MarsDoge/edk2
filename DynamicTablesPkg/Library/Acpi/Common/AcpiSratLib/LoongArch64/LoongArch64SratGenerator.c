/** @file
  LoongArch64 SRAT Table Generator helpers.

  Copyright (c) 2026, MarsDoge. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include "SratGenerator.h"

EFI_STATUS
EFIAPI
ArchReserveOffsets (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT UINT32                                           *ArchOffset
  )
{
  if ((CfgMgrProtocol == NULL) || (ArchOffset == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
AddArchObjects (
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL          *CONST  CfgMgrProtocol,
  IN EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER  *CONST  Srat
  )
{
  if ((CfgMgrProtocol == NULL) || (Srat == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
