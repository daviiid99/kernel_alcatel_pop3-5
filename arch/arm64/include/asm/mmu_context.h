/*
 * Based on arch/arm/include/asm/mmu_context.h
 *
 * Copyright (C) 1996 Russell King.
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
#ifndef __ASM_MMU_CONTEXT_H
#define __ASM_MMU_CONTEXT_H

#include <linux/compiler.h>
#include <linux/sched.h>

#include <asm/cacheflush.h>
<<<<<<< HEAD
=======
#include <asm/cpufeature.h>
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#include <asm/proc-fns.h>
#include <asm-generic/mm_hooks.h>
#include <asm/cputype.h>
#include <asm/pgtable.h>

<<<<<<< HEAD
#define MAX_ASID_BITS	16

extern unsigned int cpu_last_asid;

void __init_new_context(struct task_struct *tsk, struct mm_struct *mm);
void __new_context(struct mm_struct *mm);

=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#ifdef CONFIG_PID_IN_CONTEXTIDR
static inline void contextidr_thread_switch(struct task_struct *next)
{
	asm(
	"	msr	contextidr_el1, %0\n"
	"	isb"
	:
	: "r" (task_pid_nr(next)));
}
#else
static inline void contextidr_thread_switch(struct task_struct *next)
{
}
#endif

/*
 * Set TTBR0 to empty_zero_page. No translations will be possible via TTBR0.
 */
static inline void cpu_set_reserved_ttbr0(void)
{
<<<<<<< HEAD
	unsigned long ttbr = page_to_phys(empty_zero_page);
=======
	unsigned long ttbr = virt_to_phys(empty_zero_page);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

	asm(
	"	msr	ttbr0_el1, %0			// set TTBR0\n"
	"	isb"
	:
	: "r" (ttbr));
}

<<<<<<< HEAD
static inline void switch_new_context(struct mm_struct *mm)
{
	unsigned long flags;

	__new_context(mm);

	local_irq_save(flags);
	cpu_switch_mm(mm->pgd, mm);
	local_irq_restore(flags);
}

static inline void check_and_switch_context(struct mm_struct *mm,
					    struct task_struct *tsk)
{
	/*
	 * Required during context switch to avoid speculative page table
	 * walking with the wrong TTBR.
	 */
	cpu_set_reserved_ttbr0();

	if (!((mm->context.id ^ cpu_last_asid) >> MAX_ASID_BITS))
		/*
		 * The ASID is from the current generation, just switch to the
		 * new pgd. This condition is only true for calls from
		 * context_switch() and interrupts are already disabled.
		 */
		cpu_switch_mm(mm->pgd, mm);
	else if (irqs_disabled())
		/*
		 * Defer the new ASID allocation until after the context
		 * switch critical region since __new_context() cannot be
		 * called with interrupts disabled.
		 */
		set_ti_thread_flag(task_thread_info(tsk), TIF_SWITCH_MM);
	else
		/*
		 * That is a direct call to switch_mm() or activate_mm() with
		 * interrupts enabled and a new context.
		 */
		switch_new_context(mm);
}

#define init_new_context(tsk,mm)	(__init_new_context(tsk,mm),0)
#define destroy_context(mm)		do { } while(0)

#define finish_arch_post_lock_switch \
	finish_arch_post_lock_switch
static inline void finish_arch_post_lock_switch(void)
{
	if (test_and_clear_thread_flag(TIF_SWITCH_MM)) {
		struct mm_struct *mm = current->mm;
		unsigned long flags;

		__new_context(mm);

		local_irq_save(flags);
		cpu_switch_mm(mm->pgd, mm);
		local_irq_restore(flags);
	}
}

/*
 * This is called when "tsk" is about to enter lazy TLB mode.
 *
 * mm:  describes the currently active mm context
 * tsk: task which is entering lazy tlb
 * cpu: cpu number which is entering lazy tlb
 *
 * tsk->mm will be NULL
 */
static inline void
enter_lazy_tlb(struct mm_struct *mm, struct task_struct *tsk)
{
}

/*
 * This is the actual mm switch as far as the scheduler
 * is concerned.  No registers are touched.  We avoid
 * calling the CPU specific function when the mm hasn't
 * actually changed.
 */
static inline void
switch_mm(struct mm_struct *prev, struct mm_struct *next,
	  struct task_struct *tsk)
=======
static inline void cpu_switch_mm(pgd_t *pgd, struct mm_struct *mm)
{
	BUG_ON(pgd == swapper_pg_dir);
	cpu_set_reserved_ttbr0();
	cpu_do_switch_mm(virt_to_phys(pgd),mm);
}

/*
 * TCR.T0SZ value to use when the ID map is active. Usually equals
 * TCR_T0SZ(VA_BITS), unless system RAM is positioned very high in
 * physical memory, in which case it will be smaller.
 */
extern u64 idmap_t0sz;

static inline bool __cpu_uses_extended_idmap(void)
{
	return (!IS_ENABLED(CONFIG_ARM64_VA_BITS_48) &&
		unlikely(idmap_t0sz != TCR_T0SZ(VA_BITS)));
}

/*
 * Set TCR.T0SZ to its default value (based on VA_BITS)
 */
static inline void cpu_set_default_tcr_t0sz(void)
{
	unsigned long tcr;

	if (!__cpu_uses_extended_idmap())
		return;

	asm volatile (
	"	mrs	%0, tcr_el1	;"
	"	bfi	%0, %1, %2, %3	;"
	"	msr	tcr_el1, %0	;"
	"	isb"
	: "=&r" (tcr)
	: "r"(TCR_T0SZ(VA_BITS)), "I"(TCR_T0SZ_OFFSET), "I"(TCR_TxSZ_WIDTH));
}

/*
 * It would be nice to return ASIDs back to the allocator, but unfortunately
 * that introduces a race with a generation rollover where we could erroneously
 * free an ASID allocated in a future generation. We could workaround this by
 * freeing the ASID from the context of the dying mm (e.g. in arch_exit_mmap),
 * but we'd then need to make sure that we didn't dirty any TLBs afterwards.
 * Setting a reserved TTBR0 or EPD0 would work, but it all gets ugly when you
 * take CPU migration into account.
 */
#define destroy_context(mm)		do { } while(0)
void check_and_switch_context(struct mm_struct *mm, unsigned int cpu);

#define init_new_context(tsk,mm)	({ atomic64_set(&(mm)->context.id, 0); 0; })

#ifdef CONFIG_ARM64_SW_TTBR0_PAN
static inline void update_saved_ttbr0(struct task_struct *tsk,
				      struct mm_struct *mm)
{
	if (system_uses_ttbr0_pan()) {
		u64 ttbr;
		BUG_ON(mm->pgd == swapper_pg_dir);
		ttbr = virt_to_phys(mm->pgd) | ASID(mm) << 48;
		WRITE_ONCE(task_thread_info(tsk)->ttbr0, ttbr);
	}
}
#else
static inline void update_saved_ttbr0(struct task_struct *tsk,
				      struct mm_struct *mm)
{
}
#endif

static inline void
enter_lazy_tlb(struct mm_struct *mm, struct task_struct *tsk)
{
	/*
	 * We don't actually care about the ttbr0 mapping, so point it at the
	 * zero page.
	 */
	update_saved_ttbr0(tsk, &init_mm);
}

static inline void __switch_mm(struct mm_struct *next)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
{
	unsigned int cpu = smp_processor_id();

	/*
	 * init_mm.pgd does not contain any user mappings and it is always
	 * active for kernel addresses in TTBR1. Just set the reserved TTBR0.
	 */
	if (next == &init_mm) {
		cpu_set_reserved_ttbr0();
		return;
	}

<<<<<<< HEAD
	if (!cpumask_test_and_set_cpu(cpu, mm_cpumask(next)) || prev != next)
		check_and_switch_context(next, tsk);
}

#define deactivate_mm(tsk,mm)	do { } while (0)
#define activate_mm(prev,next)	switch_mm(prev, next, NULL)
=======
	check_and_switch_context(next, cpu);
}

static inline void
switch_mm(struct mm_struct *prev, struct mm_struct *next,
	  struct task_struct *tsk)
{
	if (prev != next)
		__switch_mm(next);

	/*
	 * Update the saved TTBR0_EL1 of the scheduled-in task as the previous
	 * value may have not been initialised yet (activate_mm caller) or the
	 * ASID has changed since the last run (following the context switch
	 * of another thread of the same process). Avoid setting the reserved
	 * TTBR0_EL1 to swapper_pg_dir (init_mm; e.g. via idle_task_exit).
	 */
	if (next != &init_mm)
		update_saved_ttbr0(tsk, next);
}

#define deactivate_mm(tsk,mm)	do { } while (0)
#define activate_mm(prev,next)	switch_mm(prev, next, current)

void post_ttbr_update_workaround(void);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916

#endif
