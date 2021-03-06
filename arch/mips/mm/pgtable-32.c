/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2003 by Ralf Baechle
 */
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/highmem.h>
#include <asm/fixmap.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>

void pgd_init(unsigned long page)
{
	unsigned long *p = (unsigned long *) page;
	int i;

	for (i = 0; i < USER_PTRS_PER_PGD; i+=8) {
		p[i + 0] = (unsigned long) invalid_pte_table;
		p[i + 1] = (unsigned long) invalid_pte_table;
		p[i + 2] = (unsigned long) invalid_pte_table;
		p[i + 3] = (unsigned long) invalid_pte_table;
		p[i + 4] = (unsigned long) invalid_pte_table;
		p[i + 5] = (unsigned long) invalid_pte_table;
		p[i + 6] = (unsigned long) invalid_pte_table;
		p[i + 7] = (unsigned long) invalid_pte_table;
	}
}

void __init pagetable_init(void)
{
	unsigned long vaddr;
	pgd_t *pgd_base;
#ifdef CONFIG_HIGHMEM
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
#endif

	/* Initialize the entire pgd.  */
	pgd_init((unsigned long)swapper_pg_dir);
	pgd_init((unsigned long)swapper_pg_dir
		 + sizeof(pgd_t) * USER_PTRS_PER_PGD);

	pgd_base = swapper_pg_dir;

	/*
	 * Fixed mappings:
	 */
<<<<<<< HEAD
	vaddr = __fix_to_virt(__end_of_fixed_addresses - 1) & PMD_MASK;
	fixrange_init(vaddr, vaddr + FIXADDR_SIZE, pgd_base);
=======
	vaddr = __fix_to_virt(__end_of_fixed_addresses - 1);
	fixrange_init(vaddr & PMD_MASK, vaddr + FIXADDR_SIZE, pgd_base);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

#ifdef CONFIG_HIGHMEM
	/*
	 * Permanent kmaps:
	 */
	vaddr = PKMAP_BASE;
<<<<<<< HEAD
	fixrange_init(vaddr, vaddr + PAGE_SIZE*LAST_PKMAP, pgd_base);
=======
	fixrange_init(vaddr & PMD_MASK, vaddr + PAGE_SIZE*LAST_PKMAP, pgd_base);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	pgd = swapper_pg_dir + __pgd_offset(vaddr);
	pud = pud_offset(pgd, vaddr);
	pmd = pmd_offset(pud, vaddr);
	pte = pte_offset_kernel(pmd, vaddr);
	pkmap_page_table = pte;
#endif
}
