/*
 * if_alg: User-space algorithm interface
 *
 * Copyright (c) 2010 Herbert Xu <herbert@gondor.apana.org.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#ifndef _CRYPTO_IF_ALG_H
#define _CRYPTO_IF_ALG_H

#include <linux/compiler.h>
#include <linux/completion.h>
#include <linux/if_alg.h>
#include <linux/scatterlist.h>
#include <linux/types.h>
#include <net/sock.h>

#define ALG_MAX_PAGES			16

struct crypto_async_request;

struct alg_sock {
	/* struct sock must be the first member of struct alg_sock */
	struct sock sk;

	struct sock *parent;

<<<<<<< HEAD
=======
	unsigned int refcnt;
	unsigned int nokey_refcnt;

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	const struct af_alg_type *type;
	void *private;
};

struct af_alg_completion {
	struct completion completion;
	int err;
};

struct af_alg_control {
	struct af_alg_iv *iv;
	int op;
};

struct af_alg_type {
	void *(*bind)(const char *name, u32 type, u32 mask);
	void (*release)(void *private);
	int (*setkey)(void *private, const u8 *key, unsigned int keylen);
	int (*accept)(void *private, struct sock *sk);
<<<<<<< HEAD

	struct proto_ops *ops;
=======
	int (*accept_nokey)(void *private, struct sock *sk);

	struct proto_ops *ops;
	struct proto_ops *ops_nokey;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	struct module *owner;
	char name[14];
};

struct af_alg_sgl {
	struct scatterlist sg[ALG_MAX_PAGES];
	struct page *pages[ALG_MAX_PAGES];
};

int af_alg_register_type(const struct af_alg_type *type);
int af_alg_unregister_type(const struct af_alg_type *type);

int af_alg_release(struct socket *sock);
<<<<<<< HEAD
=======
void af_alg_release_parent(struct sock *sk);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
int af_alg_accept(struct sock *sk, struct socket *newsock);

int af_alg_make_sg(struct af_alg_sgl *sgl, void __user *addr, int len,
		   int write);
void af_alg_free_sg(struct af_alg_sgl *sgl);

int af_alg_cmsg_send(struct msghdr *msg, struct af_alg_control *con);

int af_alg_wait_for_completion(int err, struct af_alg_completion *completion);
void af_alg_complete(struct crypto_async_request *req, int err);

static inline struct alg_sock *alg_sk(struct sock *sk)
{
	return (struct alg_sock *)sk;
}

<<<<<<< HEAD
static inline void af_alg_release_parent(struct sock *sk)
{
	sock_put(alg_sk(sk)->parent);
}

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
static inline void af_alg_init_completion(struct af_alg_completion *completion)
{
	init_completion(&completion->completion);
}

#endif	/* _CRYPTO_IF_ALG_H */
