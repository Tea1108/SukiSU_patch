# SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
# ################################################################
# Copyright (c) Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under both the BSD-style license (found in the
# LICENSE file in the root directory of this source tree) and the GPLv2 (found
# in the COPYING file in the root directory of this source tree).
# You may select, at your option, one of the above-listed licenses.
# ################################################################

GCOV_PROFILE := y
obj-$(CONFIG_CRYPTO_ZSTDN) += crypto_zstdn.o

ccflags-y  += -I$(srctree)/drivers/xiaomi/zstdn/include
crypto_zstdn-y := \
		crypto_zstd.o \
		zstd_compress_module.o \
		xxhash.o \
		common/debug.o \
		common/entropy_common.o \
		common/error_private.o \
		common/fse_decompress.o \
		common/zstd_common.o \
		compress/fse_compress.o \
		compress/hist.o \
		compress/huf_compress.o \
		compress/zstd_compress.o \
		compress/zstd_compress_literals.o \
		compress/zstd_compress_sequences.o \
		compress/zstd_compress_superblock.o \
		compress/zstd_double_fast.o \
		compress/zstd_fast.o \
		compress/zstd_lazy.o \
		compress/zstd_ldm.o \
		compress/zstd_opt.o \
		zstd_decompress_module.o \
		decompress/huf_decompress.o \
		decompress/zstd_ddict.o \
		decompress/zstd_decompress.o \
		decompress/zstd_decompress_block.o
