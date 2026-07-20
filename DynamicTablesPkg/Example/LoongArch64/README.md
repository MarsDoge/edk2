# LoongArch64 MADT Configuration Manager example

This directory contains a complete Configuration Manager producer for the
LoongArch64 MADT generator. It models the same responsibilities as a
physical-platform MADT implementation while separating platform topology from
ACPI byte-stream construction.

The example is compile-only. It is not included in `DynamicTables.fdf.inc` and
therefore is not dispatched by a firmware image unless a platform explicitly
adds the module to its FDF.

## Responsibilities

The implementation is split into two parts:

- `ConfigurationManager.c` discovers processors through
  `EFI_MP_SERVICES_PROTOCOL`, creates Core PIC objects, publishes the Standard
  and LoongArch64 Configuration Manager namespaces, and installs
  `gEdkiiConfigurationManagerProtocolGuid`.
- `PlatformMadtData.c` owns the platform interrupt-controller topology. The
  supplied data describes a representative single-node system with up to four
  logical processors and one I/O bridge.
  A real platform must replace this file with data derived from its board,
  socket, node, and bridge configuration.

This separation is intentional. The MADT generator must only serialize final CM
objects. Node-count formulas, bridge selection, link identifiers, chipset
variants, and register-address policy belong to the platform data provider.

## Physical-platform mapping

A physical LoongArch MADT producer maps to Configuration Manager objects as
follows:

| Platform MADT responsibility | Configuration Manager object |
| --- | --- |
| MADT local interrupt-controller address and flags | `CM_LOONGARCH64_MADT_INFO` |
| Processor enumeration and enabled state | `CM_LOONGARCH64_CORE_PIC_INFO[]` |
| Legacy local I/O interrupt controller | `CM_LOONGARCH64_LIO_PIC_INFO[]` |
| Extended I/O interrupt controllers and node masks | `CM_LOONGARCH64_EIO_PIC_INFO[]` |
| MSI message address and vector range | `CM_LOONGARCH64_MSI_PIC_INFO[]` |
| Per-bridge I/O interrupt controllers and GSI bases | `CM_LOONGARCH64_BIO_PIC_INFO[]` |
| LPC interrupt controller | `CM_LOONGARCH64_LPC_PIC_INFO[]` |

The current physical-platform flow does not emit an HT PIC structure. The
repository contract supports `CM_LOONGARCH64_HT_PIC_INFO[]`, but this example
returns `EFI_NOT_FOUND` because its HT list is empty.

## Processor discovery

At DXE entry the Configuration Manager:

1. locates `EFI_MP_SERVICES_PROTOCOL`;
2. obtains the processor and enabled-processor counts;
3. queries exactly processor numbers zero through the reported count minus one;
4. calls the platform `MapProcessor()` policy for every MP record;
5. creates one Core PIC CM object per record accepted by the mapper; and
6. uses the mapper-provided processor `_UID`, hardware Core ID, and flags.

MP processor-number order has no generic relationship to AML processor `_UID`
values. The sample mapper filters a non-BSP record whose hardware ID is zero and
uses a one-based filtered ordinal because that is the sample platform contract.
A real platform must map each MP hardware processor ID to the `_UID` used by its
processor device objects. Platform-specific filtering also belongs in this
mapper; the generic enumerator never accesses a processor number outside the
range reported by MP Services.
Disabled processors retain their real hardware `CoreId`. Their usability is
represented by the flags rather than a shared invalid-ID sentinel, because the
MADT generator requires every published `CoreId` to be unique. At least one
published Core PIC must have the enabled bit set.
The generated Core PIC array is allocated before the Configuration Manager
protocol is installed and remains valid for the protocol lifetime.

## Platform topology contract

`LOONGARCH64_MADT_PLATFORM_REPOSITORY` contains the table-wide MADT information,
processor mapping callback, maximum processor count, and pointer/count pairs for
every optional PIC object list. The provider owns these arrays for the lifetime
of the protocol. `MaximumProcessorCount` should be derived from the node count
and threads represented by each node.

Before CPU discovery, the module validates every optional pointer/count pair,
its descriptor-size limit, the processor-count limit, and the supported MADT
flag bits. A zero-count list must have a `NULL` pointer; a nonzero count must
have a valid pointer.

A real Loongson platform should construct the repository from its existing
configuration inputs. The physical-platform count policy maps naturally to the
pointer/count pairs: one LIO object; EIO objects selected by the interrupt mode
and active bridge topology; one MSI object per active bridge; one BIO object per
active I/O bridge; and one LPC object. The exact counts remain platform policy.
The required inputs include:

- local interrupt-controller base address;
- maximum processor count represented by the platform topology;
- active node and bridge counts;
- cores represented by each extended I/O interrupt-controller node;
- bridge/link identifiers;
- EIO node and node-mask routing;
- MSI message address and vector range;
- BIO base-address and GSI-base policy;
- chipset-dependent BIO register size; and
- LPC base address, register size, and cascade vector.

Calculate those values in the platform package, then expose only the final CM
objects to DynamicTablesPkg. Do not move those formulas into the generic MADT
generator.

The sample repository publishes all controller classes used by the physical
flow: one LIO PIC, one EIO PIC, one MSI PIC, one BIO PIC, and one LPC PIC. These
values demonstrate the object shape and must not be treated as universal board
defaults.

## DSC integration

Add the Configuration Manager module and Dynamic Tables components to the
platform DSC:

```ini
[Components.LOONGARCH64]
  DynamicTablesPkg/Example/LoongArch64/ConfigurationManagerDxe/ConfigurationManagerDxe.inf

!include DynamicTablesPkg/DynamicTables.dsc.inc
```

The platform must also provide the library instances required by
`DynamicTableManagerDxe` and `DynamicTableFactoryDxe`.

## FDF integration

The example is intentionally absent from `DynamicTables.fdf.inc`. A platform
that adopts it must add the Configuration Manager module to its own firmware
volume and include the Dynamic Tables FDF fragment:

```ini
INF DynamicTablesPkg/Example/LoongArch64/ConfigurationManagerDxe/ConfigurationManagerDxe.inf

!include DynamicTablesPkg/DynamicTables.fdf.inc
```

The Configuration Manager has a dependency on
`gEfiMpServiceProtocolGuid`. `DynamicTableManagerDxe` waits for the
Configuration Manager protocol before requesting the ACPI table list.

## Runtime flow

At runtime:

1. the platform MP Services implementation becomes available;
2. this module builds the processor-derived Core PIC repository;
3. this module installs `gEdkiiConfigurationManagerProtocolGuid`;
4. `DynamicTableManagerDxe` requests `EStdObjAcpiTableList`;
5. the MADT entry selects `EStdAcpiTableIdMadt`;
6. `DynamicTableFactoryDxe` has already registered the LoongArch64 MADT
   generator through `AcpiMadtLibLoongArch64`; and
7. the generator requests the Madt/Core/LIO/EIO/MSI/BIO/LPC objects and builds
   the final ACPI table.

The Configuration Manager returns `EFI_NOT_FOUND` for an optional PIC list when
its count is zero. The generator omits that structure class from the MADT.

## Validation

Build only the example module:

```sh
build -a LOONGARCH64 -t GCC \
  -p DynamicTablesPkg/DynamicTablesPkg.dsc \
  -m DynamicTablesPkg/Example/LoongArch64/ConfigurationManagerDxe/ConfigurationManagerDxe.inf
```

Build all LOONGARCH64 DynamicTablesPkg modules:

```sh
build -a LOONGARCH64 -t GCC \
  -p DynamicTablesPkg/DynamicTablesPkg.dsc
```

These commands validate compilation and linkage only. Platform firmware and
hardware validation must additionally compare the generated MADT with the
platform topology and verify processor and interrupt routing in the operating
system.
