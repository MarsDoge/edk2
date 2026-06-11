/** @file
  LoongArch64 SSDT CPU Topology Table Generator helpers.

  Copyright (c) 2026, MarsDoge. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <IndustryStandard/Acpi.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Library/AmlLib/AmlLib.h>
#include "SsdtCpuTopologyGenerator.h"

EFI_STATUS
EFIAPI
CreateTopologyFromIntC (
  IN        ACPI_CPU_TOPOLOGY_GENERATOR                   *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        AML_OBJECT_NODE_HANDLE                        ScopeNode
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
GetIntCInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CM_OBJECT_TOKEN                                     AcpiIdObjectToken,
  OUT UINT32                                              *AcpiProcessorUid,
  OUT CM_OBJECT_TOKEN                                     *CpcToken,
  OUT CM_OBJECT_TOKEN                                     *PsdToken
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
AddArchAmlCpuInfo (
  IN  ACPI_CPU_TOPOLOGY_GENERATOR                         *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CM_OBJECT_TOKEN                                     AcpiIdObjectToken,
  IN  UINT32                                              CpuName,
  OUT AML_OBJECT_NODE_HANDLE                              *CpuNode
  )
{
  return EFI_UNSUPPORTED;
}
