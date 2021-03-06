/*
 * Based on arch/arm/include/asm/setup.h
 *
 * Copyright (C) 1997-1999 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ASM_SETUP_H
#define __ASM_SETUP_H

#include <linux/types.h>

#define COMMAND_LINE_SIZE	2048

<<<<<<< HEAD
/* general memory descriptor */
struct mem_desc {
	u64 start;
	u64 size;
};

/* mblock is used by CPU */
struct  mblock {
	u64 start;
	u64 size;
	u32 rank;	/* rank the mblock belongs to */
};

struct mblock_info {
	u32 mblock_num;
	struct mblock mblock[4];
};

struct dram_info {
	u32 rank_num;
	struct mem_desc rank_info[4];
};

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#endif
