#ifndef _LINUX_IF_LINK_H
#define _LINUX_IF_LINK_H

#include <uapi/linux/if_link.h>


/* We don't want this structure exposed to user space */
struct ifla_vf_info {
	__u32 vf;
	__u8 mac[32];
	__u32 vlan;
	__u32 qos;
	__u32 spoofchk;
	__u32 linkstate;
	__u32 min_tx_rate;
	__u32 max_tx_rate;
<<<<<<< HEAD
=======
	__u32 rss_query_en;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
};
#endif /* _LINUX_IF_LINK_H */
