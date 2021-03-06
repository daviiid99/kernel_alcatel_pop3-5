/*
 * Hibernation support specific for ARM
 *
 * Derived from work on ARM hibernation support by:
 *
 * Ubuntu project, hibernation support for mach-dove
 * Copyright (C) 2010 Nokia Corporation (Hiroshi Doyu)
 * Copyright (C) 2010 Texas Instruments, Inc. (Teerth Reddy et al.)
 *  https://lkml.org/lkml/2010/6/18/4
 *  https://lists.linux-foundation.org/pipermail/linux-pm/2010-June/027422.html
 *  https://patchwork.kernel.org/patch/96442/
 *
 * Copyright (C) 2006 Rafael J. Wysocki <rjw@sisk.pl>
 *
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/mm.h>
#include <linux/suspend.h>
#include <asm/system_misc.h>
#include <asm/idmap.h>
#include <asm/suspend.h>
#include <asm/memory.h>
#include <asm/sections.h>
<<<<<<< HEAD
#include <mtk_hibernate_core.h>
#include "reboot.h"

#ifdef CONFIG_MTK_HIBERNATION
static int swsusp_saved;
#endif

=======
#include "reboot.h"

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
int pfn_is_nosave(unsigned long pfn)
{
	unsigned long nosave_begin_pfn = virt_to_pfn(&__nosave_begin);
	unsigned long nosave_end_pfn = virt_to_pfn(&__nosave_end - 1);

	return (pfn >= nosave_begin_pfn) && (pfn <= nosave_end_pfn);
}

void notrace save_processor_state(void)
{
	WARN_ON(num_online_cpus() != 1);
	local_fiq_disable();
<<<<<<< HEAD
#ifdef CONFIG_MTK_HIBERNATION
	mtk_save_processor_state();
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
}

void notrace restore_processor_state(void)
{
	local_fiq_enable();
<<<<<<< HEAD
#ifdef CONFIG_MTK_HIBERNATION
	mtk_restore_processor_state();
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
}

/*
 * Snapshot kernel memory and reset the system.
 *
 * swsusp_save() is executed in the suspend finisher so that the CPU
 * context pointer and memory are part of the saved image, which is
 * required by the resume kernel image to restart execution from
 * swsusp_arch_suspend().
 *
 * soft_restart is not technically needed, but is used to get success
 * returned from cpu_suspend.
 *
 * When soft reboot completes, the hibernation snapshot is written out.
 */
static int notrace arch_save_image(unsigned long unused)
{
	int ret;

	ret = swsusp_save();
<<<<<<< HEAD
#ifdef CONFIG_MTK_HIBERNATION
	swsusp_saved = (ret == 0) ? 1 : 0;
#else
	if (ret == 0)
		_soft_restart(virt_to_phys(cpu_resume), false);
#endif
=======
	if (ret == 0)
		_soft_restart(virt_to_phys(cpu_resume), false);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	return ret;
}

/*
 * Save the current CPU state before suspend / poweroff.
 */
int notrace swsusp_arch_suspend(void)
{
<<<<<<< HEAD
#ifdef CONFIG_MTK_HIBERNATION
	int retval = 0;

	retval = cpu_suspend(0, arch_save_image);
	if (swsusp_saved)
		retval = 0;

	return retval;
#else
	return cpu_suspend(0, arch_save_image);
#endif
=======
	return cpu_suspend(0, arch_save_image);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
}

/*
 * Restore page contents for physical pages that were in use during loading
 * hibernation image.  Switch to idmap_pgd so the physical page tables
 * are overwritten with the same contents.
 */
static void notrace arch_restore_image(void *unused)
{
<<<<<<< HEAD
#ifdef CONFIG_MTK_HIBERNATION
	mtk_arch_restore_image();
#else
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	struct pbe *pbe;

	cpu_switch_mm(idmap_pgd, &init_mm);
	for (pbe = restore_pblist; pbe; pbe = pbe->next)
		copy_page(pbe->orig_address, pbe->address);

	_soft_restart(virt_to_phys(cpu_resume), false);
<<<<<<< HEAD
#endif
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
}

static u64 resume_stack[PAGE_SIZE/2/sizeof(u64)] __nosavedata;

/*
 * Resume from the hibernation image.
 * Due to the kernel heap / data restore, stack contents change underneath
 * and that would make function calls impossible; switch to a temporary
 * stack within the nosave region to avoid that problem.
 */
int swsusp_arch_resume(void)
{
<<<<<<< HEAD
#ifdef CONFIG_MTK_HIBERNATION
	cpu_init();	/* get a clean PSR */
#endif
=======
	extern void call_with_stack(void (*fn)(void *), void *arg, void *sp);
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
	call_with_stack(arch_restore_image, 0,
		resume_stack + ARRAY_SIZE(resume_stack));
	return 0;
}
