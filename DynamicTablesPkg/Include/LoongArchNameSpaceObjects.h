/** @file
  LoongArch namespace object definitions.

  Copyright (c) 2026, MarsDoge. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#pragma pack(1)

typedef enum LoongArchObjectID {
  ELoongArchObjReserved,
  ELoongArchObjCorePicInfo,
  ELoongArchObjLioPicInfo,
  ELoongArchObjEioPicInfo,
  ELoongArchObjMsiPicInfo,
  ELoongArchObjBioPicInfo,
  ELoongArchObjLpcPicInfo,
  ELoongArchObjMax
} ELOONGARCH_OBJECT_ID;

typedef struct CmLoongArchCorePicInfo {
  UINT8     Version;
  UINT32    ProcessorId;
  UINT32    CoreId;
  UINT32    Flags;
} CM_LOONGARCH_CORE_PIC_INFO;

typedef struct CmLoongArchLioPicInfo {
  UINT8     Version;
  UINT64    Address;
  UINT16    Size;
  UINT8     Cascade[2];
  UINT32    CascadeMap[2];
} CM_LOONGARCH_LIO_PIC_INFO;

typedef struct CmLoongArchEioPicInfo {
  UINT8     Version;
  UINT8     Cascade;
  UINT8     Node;
  UINT64    NodeMap;
} CM_LOONGARCH_EIO_PIC_INFO;

typedef struct CmLoongArchMsiPicInfo {
  UINT8     Version;
  UINT64    MsgAddress;
  UINT32    Start;
  UINT32    Count;
} CM_LOONGARCH_MSI_PIC_INFO;

typedef struct CmLoongArchBioPicInfo {
  UINT8     Version;
  UINT64    Address;
  UINT16    Size;
  UINT16    Id;
  UINT16    GsiBase;
} CM_LOONGARCH_BIO_PIC_INFO;

typedef struct CmLoongArchLpcPicInfo {
  UINT8     Version;
  UINT64    Address;
  UINT16    Size;
  UINT8     Cascade;
} CM_LOONGARCH_LPC_PIC_INFO;

#pragma pack()
