#ifndef __SOCK_DIAG_H__
#define __SOCK_DIAG_H__

#include <linux/user_namespace.h>
#include <uapi/linux/sock_diag.h>

struct sk_buff;
struct nlmsghdr;
struct sock;

struct sock_diag_handler {
	__u8 family;
	int (*dump)(struct sk_buff *skb, struct nlmsghdr *nlh);
<<<<<<< HEAD
=======
	int (*destroy)(struct sk_buff *skb, struct nlmsghdr *nlh);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
};

int sock_diag_register(const struct sock_diag_handler *h);
void sock_diag_unregister(const struct sock_diag_handler *h);

void sock_diag_register_inet_compat(int (*fn)(struct sk_buff *skb, struct nlmsghdr *nlh));
void sock_diag_unregister_inet_compat(int (*fn)(struct sk_buff *skb, struct nlmsghdr *nlh));

int sock_diag_check_cookie(void *sk, __u32 *cookie);
void sock_diag_save_cookie(void *sk, __u32 *cookie);

int sock_diag_put_meminfo(struct sock *sk, struct sk_buff *skb, int attr);
int sock_diag_put_filterinfo(bool may_report_filterinfo, struct sock *sk,
			     struct sk_buff *skb, int attrtype);

<<<<<<< HEAD
=======
int sock_diag_destroy(struct sock *sk, int err);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#endif
