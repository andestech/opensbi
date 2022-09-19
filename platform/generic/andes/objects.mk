#
# SPDX-License-Identifier: BSD-2-Clause
#

carray-platform_override_modules-$(CONFIG_PLATFORM_ANDES_AE350) += andes_ae350
platform-objs-$(CONFIG_PLATFORM_ANDES_AE350) += andes/ae350.o andes/sleep.o andes/pmu.o
platform-objs-$(CONFIG_ANDES_TRIGGER) += andes/trigger.o
platform-objs-$(CONFIG_ANDES_PMA) += andes/pma.o
platform-objs-$(CONFIG_ANDES_CACHE) += andes/cache.o
