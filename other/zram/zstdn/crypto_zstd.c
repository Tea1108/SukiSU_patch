// SPDX-License-Identifier: GPL-2.0-only
/*
 * Cryptographic API.
 *
 * Copyright (c) 2017-present, Facebook, Inc.
 * the source is changed by oplus for out of tree module.
 * replace zstd -> zstdn n stands for newest
 */
#include <linux/crypto.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/net.h>
#include <linux/vmalloc.h>
#include "include/zstd.h"
#include <crypto/internal/scompress.h>


#define ZSTD_DEF_LEVEL	1

struct zstd_ctx {
	zstd_cctx *cctx;
	zstd_dctx *dctx;
	void *cwksp;
	void *dwksp;
};

static zstd_parameters zstd_params(void)
{
	return zstd_get_params(ZSTD_DEF_LEVEL, 0);
}

static int zstd_comp_init(struct zstd_ctx *ctx)
{
	int ret = 0;
	const zstd_parameters params = zstd_params();
	const size_t wksp_size = zstd_cctx_workspace_bound(&params.cParams);

	ctx->cwksp = vzalloc(wksp_size);
	if (!ctx->cwksp) {
		ret = -ENOMEM;
		goto out;
	}

	ctx->cctx = zstd_init_cctx(ctx->cwksp, wksp_size);
	if (!ctx->cctx) {
		ret = -EINVAL;
		goto out_free;
	}
out:
	return ret;
out_free:
	vfree(ctx->cwksp);
	goto out;
}

static int zstd_decomp_init(struct zstd_ctx *ctx)
{
	int ret = 0;
	const size_t wksp_size = zstd_dctx_workspace_bound();

	ctx->dwksp = vzalloc(wksp_size);
	if (!ctx->dwksp) {
		ret = -ENOMEM;
		goto out;
	}

	ctx->dctx = zstd_init_dctx(ctx->dwksp, wksp_size);
	if (!ctx->dctx) {
		ret = -EINVAL;
		goto out_free;
	}
out:
	return ret;
out_free:
	vfree(ctx->dwksp);
	goto out;
}

static void zstd_comp_exit(struct zstd_ctx *ctx)
{
	vfree(ctx->cwksp);
	ctx->cwksp = NULL;
	ctx->cctx = NULL;
}

static void zstd_decomp_exit(struct zstd_ctx *ctx)
{
	vfree(ctx->dwksp);
	ctx->dwksp = NULL;
	ctx->dctx = NULL;
}

static int __zstd_init(void *ctx)
{
	int ret;

	ret = zstd_comp_init(ctx);
	if (ret)
		return ret;
	ret = zstd_decomp_init(ctx);
	if (ret)
		zstd_comp_exit(ctx);
	return ret;
}

static void *zstd_alloc_ctx(struct crypto_scomp *tfm)
{
	int ret;
	struct zstd_ctx *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	ret = __zstd_init(ctx);
	if (ret) {
		kfree(ctx);
		return ERR_PTR(ret);
	}

	return ctx;
}

static int zstd_init(struct crypto_tfm *tfm)
{
	struct zstd_ctx *ctx = crypto_tfm_ctx(tfm);

	return __zstd_init(ctx);
}

static void __zstd_exit(void *ctx)
{
	zstd_comp_exit(ctx);
	zstd_decomp_exit(ctx);
}

static void zstd_free_ctx(struct crypto_scomp *tfm, void *ctx)
{
	__zstd_exit(ctx);
	kfree(ctx);
}

static void zstd_exit(struct crypto_tfm *tfm)
{
	struct zstd_ctx *ctx = crypto_tfm_ctx(tfm);

	__zstd_exit(ctx);
}

static int __zstd_compress(const u8 *src, unsigned int slen,
			   u8 *dst, unsigned int *dlen, void *ctx)
{
	size_t out_len;
	struct zstd_ctx *zctx = ctx;
	const zstd_parameters params = zstd_params();

	out_len = zstd_compress_cctx(zctx->cctx, dst, *dlen, src, slen, &params);
	if (zstd_is_error(out_len))
		return -EINVAL;
	*dlen = out_len;
	return 0;
}

static int zstd_compress(struct crypto_tfm *tfm, const u8 *src,
			 unsigned int slen, u8 *dst, unsigned int *dlen)
{
	struct zstd_ctx *ctx = crypto_tfm_ctx(tfm);

	return __zstd_compress(src, slen, dst, dlen, ctx);
}

static int zstd_scompress(struct crypto_scomp *tfm, const u8 *src,
			  unsigned int slen, u8 *dst, unsigned int *dlen,
			  void *ctx)
{
	return __zstd_compress(src, slen, dst, dlen, ctx);
}

static int __zstd_decompress(const u8 *src, unsigned int slen,
			     u8 *dst, unsigned int *dlen, void *ctx)
{
	size_t out_len;
	struct zstd_ctx *zctx = ctx;

	out_len = zstd_decompress_dctx(zctx->dctx, dst, *dlen, src, slen);
	if (zstd_is_error(out_len))
		return -EINVAL;
	*dlen = out_len;
	return 0;
}

static int zstd_decompress(struct crypto_tfm *tfm, const u8 *src,
			   unsigned int slen, u8 *dst, unsigned int *dlen)
{
	struct zstd_ctx *ctx = crypto_tfm_ctx(tfm);

	return __zstd_decompress(src, slen, dst, dlen, ctx);
}

static int zstd_sdecompress(struct crypto_scomp *tfm, const u8 *src,
			    unsigned int slen, u8 *dst, unsigned int *dlen,
			    void *ctx)
{
	return __zstd_decompress(src, slen, dst, dlen, ctx);
}

static struct crypto_alg alg = {
	.cra_name		= "zstdn",
	.cra_driver_name	= "zstdn-generic",
	.cra_flags		= CRYPTO_ALG_TYPE_COMPRESS,
	.cra_ctxsize		= sizeof(struct zstd_ctx),
	.cra_module		= THIS_MODULE,
	.cra_init		= zstd_init,
	.cra_exit		= zstd_exit,
	.cra_u			= { .compress = {
	.coa_compress		= zstd_compress,
	.coa_decompress		= zstd_decompress } }
};

static struct scomp_alg scomp = {
	.alloc_ctx		= zstd_alloc_ctx,
	.free_ctx		= zstd_free_ctx,
	.compress		= zstd_scompress,
	.decompress		= zstd_sdecompress,
	.base			= {
		.cra_name	= "zstdn",
		.cra_driver_name = "zstdn-scomp",
		.cra_module	 = THIS_MODULE,
	}
};

static int __init zstdn_mod_init(void)
{
	int ret;

	pr_info("register comp zstdn start\n");
	ret = crypto_register_alg(&alg);
	if (ret)
		return ret;

	ret = crypto_register_scomp(&scomp);
	if (ret)
		crypto_unregister_alg(&alg);
	pr_info("register comp zstdn success\n");

	return ret;
}

static void __exit zstdn_mod_fini(void)
{
	crypto_unregister_alg(&alg);
	crypto_unregister_scomp(&scomp);
}

subsys_initcall(zstdn_mod_init);
module_exit(zstdn_mod_fini);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Zstd Compression Algorithm");
MODULE_ALIAS_CRYPTO("zstdn");
