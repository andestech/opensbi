#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2023 Andes Technology Corporation
#
# Authors:
#   Yu Chien Peter Lin <peterlin@andestech.com>
#

libsbiutils-objs-$(CONFIG_CACHE) += cache/cache.o

libsbiutils-objs-$(CONFIG_FDT_CACHE) += cache/fdt_cache.o
libsbiutils-objs-$(CONFIG_FDT_CACHE) += cache/fdt_cache_drivers.o

carray-fdt_cache_drivers-$(CONFIG_FDT_CACHE_ANDES_L2C) += fdt_cache_andes_l2c
libsbiutils-objs-$(CONFIG_FDT_CACHE_ANDES_L2C) += cache/fdt_cache_andes_l2c.o
