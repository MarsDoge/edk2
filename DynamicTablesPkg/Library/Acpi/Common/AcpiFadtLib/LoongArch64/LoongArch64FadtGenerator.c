/** @file
  LoongArch64 FADT Table Generator helpers.

  Copyright (c) 2026, MarsDoge. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "FadtGenerator.h"

EFI_STATUS
EFIAPI
FadtArchUpdate (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  OUT EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE      *Fadt
  )
{
  if ((CfgMgrProtocol == NULL) || (Fadt == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
