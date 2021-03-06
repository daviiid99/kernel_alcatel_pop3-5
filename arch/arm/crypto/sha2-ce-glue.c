/*
 * sha2-ce-glue.c - SHA-224/SHA-256 using ARMv8 Crypto Extensions
 *
 * Copyright (C) 2015 Linaro Ltd <ard.biesheuvel@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <crypto/internal/hash.h>
#include <crypto/sha.h>
<<<<<<< HEAD
#include <crypto/sha256_base.h>
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#include <linux/crypto.h>
#include <linux/module.h>

#include <asm/hwcap.h>
#include <asm/simd.h>
#include <asm/neon.h>
#include <asm/unaligned.h>

<<<<<<< HEAD
#include "sha256_glue.h"

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
MODULE_DESCRIPTION("SHA-224/SHA-256 secure hash using ARMv8 Crypto Extensions");
MODULE_AUTHOR("Ard Biesheuvel <ard.biesheuvel@linaro.org>");
MODULE_LICENSE("GPL v2");

<<<<<<< HEAD
asmlinkage void sha2_ce_transform(struct sha256_state *sst, u8 const *src,
				  int blocks);

static int sha2_ce_update(struct shash_desc *desc, const u8 *data,
			  unsigned int len)
{
	struct sha256_state *sctx = shash_desc_ctx(desc);

	if (!may_use_simd() ||
	    (sctx->count % SHA256_BLOCK_SIZE) + len < SHA256_BLOCK_SIZE)
		return crypto_sha256_arm_update(desc, data, len);

	kernel_neon_begin();
	sha256_base_do_update(desc, data, len,
			      (sha256_block_fn *)sha2_ce_transform);
	kernel_neon_end();

	return 0;
}

static int sha2_ce_finup(struct shash_desc *desc, const u8 *data,
			 unsigned int len, u8 *out)
{
	if (!may_use_simd())
		return crypto_sha256_arm_finup(desc, data, len, out);

	kernel_neon_begin();
	if (len)
		sha256_base_do_update(desc, data, len,
				      (sha256_block_fn *)sha2_ce_transform);
	sha256_base_do_finalize(desc, (sha256_block_fn *)sha2_ce_transform);
	kernel_neon_end();

	return sha256_base_finish(desc, out);
}

static int sha2_ce_final(struct shash_desc *desc, u8 *out)
{
	return sha2_ce_finup(desc, NULL, 0, out);
}

static struct shash_alg algs[] = { {
	.init			= sha224_base_init,
	.update			= sha2_ce_update,
	.final			= sha2_ce_final,
	.finup			= sha2_ce_finup,
	.descsize		= sizeof(struct sha256_state),
	.digestsize		= SHA224_DIGEST_SIZE,
=======
asmlinkage void sha2_ce_transform(int blocks, u8 const *src, u32 *state,
				  u8 *head);

static int sha224_init(struct shash_desc *desc)
{
	struct sha256_state *sctx = shash_desc_ctx(desc);

	*sctx = (struct sha256_state){
		.state = {
			SHA224_H0, SHA224_H1, SHA224_H2, SHA224_H3,
			SHA224_H4, SHA224_H5, SHA224_H6, SHA224_H7,
		}
	};
	return 0;
}

static int sha256_init(struct shash_desc *desc)
{
	struct sha256_state *sctx = shash_desc_ctx(desc);

	*sctx = (struct sha256_state){
		.state = {
			SHA256_H0, SHA256_H1, SHA256_H2, SHA256_H3,
			SHA256_H4, SHA256_H5, SHA256_H6, SHA256_H7,
		}
	};
	return 0;
}

static int sha2_update(struct shash_desc *desc, const u8 *data,
		       unsigned int len)
{
	struct sha256_state *sctx = shash_desc_ctx(desc);
	unsigned int partial;

	if (!may_use_simd())
		return crypto_sha256_update(desc, data, len);

	partial = sctx->count % SHA256_BLOCK_SIZE;
	sctx->count += len;

	if ((partial + len) >= SHA256_BLOCK_SIZE) {
		int blocks;

		if (partial) {
			int p = SHA256_BLOCK_SIZE - partial;

			memcpy(sctx->buf + partial, data, p);
			data += p;
			len -= p;
		}

		blocks = len / SHA256_BLOCK_SIZE;
		len %= SHA256_BLOCK_SIZE;

		kernel_neon_begin();
		sha2_ce_transform(blocks, data, sctx->state,
				  partial ? sctx->buf : NULL);
		kernel_neon_end();

		data += blocks * SHA256_BLOCK_SIZE;
		partial = 0;
	}
	if (len)
		memcpy(sctx->buf + partial, data, len);
	return 0;
}

static void sha2_final(struct shash_desc *desc)
{
	static const u8 padding[SHA256_BLOCK_SIZE] = { 0x80, };

	struct sha256_state *sctx = shash_desc_ctx(desc);
	__be64 bits = cpu_to_be64(sctx->count << 3);
	u32 padlen = SHA256_BLOCK_SIZE
		     - ((sctx->count + sizeof(bits)) % SHA256_BLOCK_SIZE);

	sha2_update(desc, padding, padlen);
	sha2_update(desc, (const u8 *)&bits, sizeof(bits));
}

static int sha224_final(struct shash_desc *desc, u8 *out)
{
	struct sha256_state *sctx = shash_desc_ctx(desc);
	__be32 *dst = (__be32 *)out;
	int i;

	sha2_final(desc);

	for (i = 0; i < SHA224_DIGEST_SIZE / sizeof(__be32); i++)
		put_unaligned_be32(sctx->state[i], dst++);

	*sctx = (struct sha256_state){};
	return 0;
}

static int sha256_final(struct shash_desc *desc, u8 *out)
{
	struct sha256_state *sctx = shash_desc_ctx(desc);
	__be32 *dst = (__be32 *)out;
	int i;

	sha2_final(desc);

	for (i = 0; i < SHA256_DIGEST_SIZE / sizeof(__be32); i++)
		put_unaligned_be32(sctx->state[i], dst++);

	*sctx = (struct sha256_state){};
	return 0;
}

static int sha2_export(struct shash_desc *desc, void *out)
{
	struct sha256_state *sctx = shash_desc_ctx(desc);
	struct sha256_state *dst = out;

	*dst = *sctx;
	return 0;
}

static int sha2_import(struct shash_desc *desc, const void *in)
{
	struct sha256_state *sctx = shash_desc_ctx(desc);
	struct sha256_state const *src = in;

	*sctx = *src;
	return 0;
}

static struct shash_alg algs[] = { {
	.init			= sha224_init,
	.update			= sha2_update,
	.final			= sha224_final,
	.export			= sha2_export,
	.import			= sha2_import,
	.descsize		= sizeof(struct sha256_state),
	.digestsize		= SHA224_DIGEST_SIZE,
	.statesize		= sizeof(struct sha256_state),
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	.base			= {
		.cra_name		= "sha224",
		.cra_driver_name	= "sha224-ce",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_SHASH,
		.cra_blocksize		= SHA256_BLOCK_SIZE,
		.cra_module		= THIS_MODULE,
	}
}, {
<<<<<<< HEAD
	.init			= sha256_base_init,
	.update			= sha2_ce_update,
	.final			= sha2_ce_final,
	.finup			= sha2_ce_finup,
	.descsize		= sizeof(struct sha256_state),
	.digestsize		= SHA256_DIGEST_SIZE,
=======
	.init			= sha256_init,
	.update			= sha2_update,
	.final			= sha256_final,
	.export			= sha2_export,
	.import			= sha2_import,
	.descsize		= sizeof(struct sha256_state),
	.digestsize		= SHA256_DIGEST_SIZE,
	.statesize		= sizeof(struct sha256_state),
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	.base			= {
		.cra_name		= "sha256",
		.cra_driver_name	= "sha256-ce",
		.cra_priority		= 300,
		.cra_flags		= CRYPTO_ALG_TYPE_SHASH,
		.cra_blocksize		= SHA256_BLOCK_SIZE,
		.cra_module		= THIS_MODULE,
	}
} };

static int __init sha2_ce_mod_init(void)
{
	if (!(elf_hwcap2 & HWCAP2_SHA2))
		return -ENODEV;
	return crypto_register_shashes(algs, ARRAY_SIZE(algs));
}

static void __exit sha2_ce_mod_fini(void)
{
	crypto_unregister_shashes(algs, ARRAY_SIZE(algs));
}

module_init(sha2_ce_mod_init);
module_exit(sha2_ce_mod_fini);
