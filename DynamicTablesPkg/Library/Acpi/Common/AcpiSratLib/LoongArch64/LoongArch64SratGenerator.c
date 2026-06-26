/** @file
  LoongArch64 SRAT Table Helpers

  Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.5 Specification, Aug 29, 2022
**/

#include <IndustryStandard/Acpi65.h>
#include <Library/DebugLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "../SratGenerator.h"

/** Reserve arch sub-tables space.

  @param [in] CfgMgrProtocol   Pointer to the Configuration Manager.
  @param [in, out] ArchOffset  Offset at which arch specific sub-tables start.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
EFIAPI
ArchReserveOffsets (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT UINT32                                           *ArchOffset
  )
{
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (ArchOffset != NULL);

  if ((CfgMgrProtocol == NULL) || (ArchOffset == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Add the arch specific sub-tables to the SRAT table.

  @param [in] CfgMgrProtocol   Pointer to the Configuration Manager.
  @param [in] Srat             Pointer to the SRAT Table.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
EFI_STATUS
EFIAPI
AddArchObjects (
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL          *CONST  CfgMgrProtocol,
  IN EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER  *CONST  Srat
  )
{
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Srat != NULL);

  if ((CfgMgrProtocol == NULL) || (Srat == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
