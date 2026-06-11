/** @file
  LoongArch64 DBG2 Table Generator helpers.

  Copyright (c) 2026, MarsDoge. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <ConfigurationManagerObject.h>
#include <Protocol/SerialIo.h>
#include "Dbg2Generator.h"

RETURN_STATUS
EFIAPI
Dbg2InitializePort (
  IN  CONST CM_ARCH_COMMON_SERIAL_PORT_INFO  *SerialPortInfo,
  IN OUT UINT64                              *BaudRate,
  IN OUT UINT32                              *ReceiveFifoDepth,
  IN OUT EFI_PARITY_TYPE                     *Parity,
  IN OUT UINT8                               *DataBits,
  IN OUT EFI_STOP_BITS_TYPE                  *StopBits
  )
{
  return RETURN_SUCCESS;
}
