/*
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
#ifndef __ASM_MMU_H
#define __ASM_MMU_H

<<<<<<< HEAD
typedef struct {
	unsigned int id;
	raw_spinlock_t id_lock;
	void *vdso;
} mm_context_t;

#define INIT_MM_CONTEXT(name) \
	.context.id_lock = __RAW_SPIN_LOCK_UNLOCKED(name.context.id_lock),

#define ASID(mm)	((mm)->context.id & 0xffff)

extern void paging_init(void);
extern void setup_mm_for_reboot(void);
extern void __iomem *early_io_map(phys_addr_t phys, unsigned long virt);
extern void init_mem_pgprot(void);
/* create an identity mapping for memory (or io if map_io is true) */
extern void create_id_mapping(phys_addr_t addr, phys_addr_t size, int map_io);

=======
#define USER_ASID_FLAG	(UL(1) << 48)
#define TTBR_ASID_MASK	(UL(0xffff) << 48)

#ifndef __ASSEMBLY__

#include <asm/cpufeature.h>

typedef struct {
	atomic64_t	id;
	void		*vdso;
} mm_context_t;

/*
 * This macro is only used by the TLBI code, which cannot race with an
 * ASID change and therefore doesn't need to reload the counter using
 * atomic64_read.
 */
#define ASID(mm)	((mm)->context.id.counter & 0xffff)

static inline bool arm64_kernel_unmapped_at_el0(void)
{
	return IS_ENABLED(CONFIG_UNMAP_KERNEL_AT_EL0) &&
	       cpus_have_cap(ARM64_UNMAP_KERNEL_AT_EL0);
}

extern void paging_init(void);
extern void __iomem *early_io_map(phys_addr_t phys, unsigned long virt);
extern void init_mem_pgprot(void);
extern void create_pgd_mapping(struct mm_struct *mm, phys_addr_t phys,
			       unsigned long virt, phys_addr_t size,
			       pgprot_t prot);

#endif	/* !__ASSEMBLY__ */
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#endif
