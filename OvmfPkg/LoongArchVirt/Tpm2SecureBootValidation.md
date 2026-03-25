# LoongArch TPM2 Secure Boot Validation Guide

## Scope

This guide validates the `OvmfPkg/LoongArchVirt` firmware with:

- TPM2 discovery and TCG2 protocol support
- Measured boot event logging
- Secure Boot image verification
- QEMU `loongarch/virt` integration through `tpm-tis-device`

The flow below assumes:

- `edk2` is built from this tree
- `qemu-system-loongarch64` supports `tpm-tis-device`
- `swtpm` is available on the host

## Build

Build a firmware image with both Secure Boot and TPM2 enabled:

```bash
export WORKSPACE=$PWD
export PACKAGES_PATH=$WORKSPACE/edk2
export EDK_TOOLS_PATH=$WORKSPACE/edk2/BaseTools
export GCC_LOONGARCH64_PREFIX=loongarch64-unknown-linux-gnu-

source edk2/edksetup.sh --reconfig
make -C edk2/BaseTools
source edk2/edksetup.sh BaseTools

build -b RELEASE -t GCC -a LOONGARCH64 \
  -p OvmfPkg/LoongArchVirt/LoongArchVirtQemu.dsc \
  -D SECURE_BOOT_ENABLE=TRUE \
  -D SECURE_BOOT_DEFAULT_KEYS=TRUE \
  -D TPM2_ENABLE=TRUE \
  -D TPM2_CONFIG_ENABLE=TRUE
```

Expected build artifacts:

- `Build/LoongArchVirtQemu/RELEASE_GCC/FV/QEMU_EFI.fd`
- `Build/LoongArchVirtQemu/RELEASE_GCC/FV/QEMU_VARS.fd`

## Host TPM Emulator

Create a persistent software TPM state directory and start `swtpm`:

```bash
mkdir -p /tmp/loongarch-swtpm
rm -f /tmp/loongarch-swtpm/swtpm-sock

swtpm socket \
  --tpm2 \
  --tpmstate dir=/tmp/loongarch-swtpm \
  --ctrl type=unixio,path=/tmp/loongarch-swtpm/swtpm-sock \
  --daemon
```

## QEMU Launch

Use split pflash images so Secure Boot variables persist across reboots:

```bash
cp Build/LoongArchVirtQemu/RELEASE_GCC/FV/QEMU_VARS.fd /tmp/loongarch-vars.fd

qemu-system-loongarch64 \
  -M virt \
  -cpu la464 \
  -smp 4 \
  -m 4G \
  -drive if=pflash,format=raw,unit=0,file=Build/LoongArchVirtQemu/RELEASE_GCC/FV/QEMU_EFI.fd,readonly=on \
  -drive if=pflash,format=raw,unit=1,file=/tmp/loongarch-vars.fd \
  -chardev socket,id=chrtpm,path=/tmp/loongarch-swtpm/swtpm-sock \
  -tpmdev emulator,id=tpm0,chardev=chrtpm \
  -device tpm-tis-device,tpmdev=tpm0 \
  -serial mon:stdio
```

Add your boot disk as needed, for example:

```bash
  -drive file=loongarch-guest.qcow2,if=virtio,format=qcow2
```

## Firmware Checks

From the UEFI front page or shell, confirm Secure Boot state:

```text
dmpstore SecureBoot
dmpstore PK
dmpstore KEK
dmpstore db
```

Expected result:

- `SecureBoot` is present and set to `01`
- `PK`, `KEK`, and `db` are populated when `SECURE_BOOT_DEFAULT_KEYS=TRUE`

Confirm TPM2 is exposed:

```text
drivers
devices
```

Expected result:

- `Tcg2Dxe` is loaded
- no TPM initialization error is printed during DXE

## Guest OS Checks

Inside a Linux guest with `tpm2-tools` installed:

```bash
ls /dev/tpm0
ls /sys/class/tpm/tpm0
tpm2_getcap properties-fixed
tpm2_pcrread sha256:0,1,7
```

Expected result:

- `/dev/tpm0` exists
- TPM capabilities report a TPM 2.0 device
- PCRs can be read successfully

Check the firmware event log:

```bash
ls /sys/kernel/security/tpm0/binary_bios_measurements
```

If available, dump it with:

```bash
tpm2_eventlog /sys/kernel/security/tpm0/binary_bios_measurements
```

Expected measurements:

- firmware and FV measurements in early PCRs
- Secure Boot policy / signature database measurements in PCR 7

## Negative Secure Boot Test

To verify image rejection, try one of the following:

- boot an unsigned EFI application from the UEFI shell
- boot a kernel or bootloader signed by a key not present in `db`

Expected result:

- firmware refuses to execute the image
- the failure is reported as a Secure Boot violation
- PCR 7 remains consistent with the enforced Secure Boot policy

## Regression Checklist

- Boot with TPM2 disabled: `-D TPM2_ENABLE=FALSE`
- Boot with Secure Boot disabled: `-D SECURE_BOOT_ENABLE=FALSE`
- Boot with TPM2 enabled but no `tpm-tis-device` on the QEMU command line

Expected result:

- firmware still boots
- TPM stack is skipped cleanly when no TPM device is present
- Secure Boot behavior only changes when explicitly enabled
