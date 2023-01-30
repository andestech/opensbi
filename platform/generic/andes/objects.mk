#
# SPDX-License-Identifier: BSD-2-Clause
#

# Use Andes custom configuration
PLATFORM_DEFCONFIG=andes_defconfig

# Blobs to build
FW_TEXT_START=0x00000000
FW_JUMP=n
FW_PAYLOAD=n

# Objects to build
carray-platform_override_modules-$(CONFIG_PLATFORM_ANDES_AE350) += andes_ae350
platform-objs-$(CONFIG_PLATFORM_ANDES_AE350) += andes/ae350.o andes/sleep.o

platform-objs-$(CONFIG_ANDES_PMA) += andes/andes_pma.o
platform-objs-$(CONFIG_ANDES_SBI) += andes/andes_sbi.o
platform-objs-$(CONFIG_ANDES_PMU) += andes/andes_pmu.o
platform-objs-$(CONFIG_ANDES_TRIGGER) += andes/trigger.o
