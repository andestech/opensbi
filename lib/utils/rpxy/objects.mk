#
# SPDX-License-Identifier: BSD-2-Clause
#
# Copyright (c) 2023 Ventana Micro Systems Inc.
#
# Authors:
#   Anup Patel <apatel@ventanamicro.com>
#

libsbiutils-objs-$(CONFIG_FDT_RPXY) += rpxy/fdt_rpxy.o
libsbiutils-objs-$(CONFIG_FDT_RPXY) += rpxy/fdt_rpxy_drivers.o
carray-fdt_rpxy_drivers-$(CONFIG_FDT_RPXY_SPD) += fdt_rpxy_spd
libsbiutils-objs-$(CONFIG_FDT_RPXY_SPD) += rpxy/fdt_rpxy_spd.o
