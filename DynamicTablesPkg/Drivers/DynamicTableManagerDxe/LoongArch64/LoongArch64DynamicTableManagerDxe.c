/** @file
  LoongArch64 Dynamic Table Manager Dxe.

  Copyright (c) 2026, MarsDoge. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include "DynamicTableManagerDxe.h"

STATIC ACPI_TABLE_PRESENCE_INFO  mAcpiVerifyTables[] = {
  { EStdAcpiTableIdFadt, EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,            "FADT", TRUE,  0 },
  { EStdAcpiTableIdMadt, EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,         "MADT", TRUE,  0 },
  { EStdAcpiTableIdDsdt, EFI_ACPI_6_5_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE, "DSDT", TRUE,  0 },
  { EStdAcpiTableIdDbg2, EFI_ACPI_6_5_DEBUG_PORT_2_TABLE_SIGNATURE,                      "DBG2", FALSE, 0 },
  { EStdAcpiTableIdSpcr, EFI_ACPI_6_5_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,   "SPCR", FALSE, 0 },
};

EFI_STATUS
EFIAPI
GetAcpiTablePresenceInfo (
  OUT ACPI_TABLE_PRESENCE_INFO  **PresenceArray,
  OUT UINT32                    *PresenceArrayCount,
  OUT INT32                     *FadtIndex
  )
{
  *PresenceArray      = mAcpiVerifyTables;
  *PresenceArrayCount = ARRAY_SIZE (mAcpiVerifyTables);
  *FadtIndex          = ACPI_TABLE_VERIFY_FADT;

  return EFI_SUCCESS;
}
