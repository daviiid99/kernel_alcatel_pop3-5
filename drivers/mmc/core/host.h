/*
 *  linux/drivers/mmc/core/host.h
 *
 *  Copyright (C) 2003 Russell King, All Rights Reserved.
 *  Copyright 2007 Pierre Ossman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _MMC_CORE_HOST_H
#define _MMC_CORE_HOST_H
#include <linux/mmc/host.h>

<<<<<<< HEAD
int mmc_register_host_class(void);
void mmc_unregister_host_class(void);

=======
#define cls_dev_to_mmc_host(d)	container_of(d, struct mmc_host, class_dev)

int mmc_register_host_class(void);
void mmc_unregister_host_class(void);

#ifdef CONFIG_BLOCK
void mmc_latency_hist_sysfs_init(struct mmc_host *host);
void mmc_latency_hist_sysfs_exit(struct mmc_host *host);
#endif

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#endif

