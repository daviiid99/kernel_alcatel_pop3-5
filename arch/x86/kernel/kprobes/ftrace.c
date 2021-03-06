/*
 * Dynamic Ftrace based Kprobes Optimization
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) Hitachi Ltd., 2012
 */
#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <linux/ftrace.h>

#include "common.h"

static nokprobe_inline
<<<<<<< HEAD
int __skip_singlestep(struct kprobe *p, struct pt_regs *regs,
=======
void __skip_singlestep(struct kprobe *p, struct pt_regs *regs,
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		      struct kprobe_ctlblk *kcb)
{
	/*
	 * Emulate singlestep (and also recover regs->ip)
	 * as if there is a 5byte nop
	 */
	regs->ip = (unsigned long)p->addr + MCOUNT_INSN_SIZE;
	if (unlikely(p->post_handler)) {
		kcb->kprobe_status = KPROBE_HIT_SSDONE;
		p->post_handler(p, regs, 0);
	}
	__this_cpu_write(current_kprobe, NULL);
<<<<<<< HEAD
	return 1;
=======
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
}

int skip_singlestep(struct kprobe *p, struct pt_regs *regs,
		    struct kprobe_ctlblk *kcb)
{
<<<<<<< HEAD
	if (kprobe_ftrace(p))
		return __skip_singlestep(p, regs, kcb);
	else
		return 0;
}
NOKPROBE_SYMBOL(skip_singlestep);

/* Ftrace callback handler for kprobes */
=======
	if (kprobe_ftrace(p)) {
		__skip_singlestep(p, regs, kcb);
		preempt_enable_no_resched();
		return 1;
	}
	return 0;
}
NOKPROBE_SYMBOL(skip_singlestep);

/* Ftrace callback handler for kprobes -- called under preepmt disabed */
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
void kprobe_ftrace_handler(unsigned long ip, unsigned long parent_ip,
			   struct ftrace_ops *ops, struct pt_regs *regs)
{
	struct kprobe *p;
	struct kprobe_ctlblk *kcb;
	unsigned long flags;

	/* Disable irq for emulating a breakpoint and avoiding preempt */
	local_irq_save(flags);

	p = get_kprobe((kprobe_opcode_t *)ip);
	if (unlikely(!p) || kprobe_disabled(p))
		goto end;

	kcb = get_kprobe_ctlblk();
	if (kprobe_running()) {
		kprobes_inc_nmissed_count(p);
	} else {
		/* Kprobe handler expects regs->ip = ip + 1 as breakpoint hit */
		regs->ip = ip + sizeof(kprobe_opcode_t);

<<<<<<< HEAD
		__this_cpu_write(current_kprobe, p);
		kcb->kprobe_status = KPROBE_HIT_ACTIVE;
		if (!p->pre_handler || !p->pre_handler(p, regs))
			__skip_singlestep(p, regs, kcb);
		/*
		 * If pre_handler returns !0, it sets regs->ip and
		 * resets current kprobe.
=======
		/* To emulate trap based kprobes, preempt_disable here */
		preempt_disable();
		__this_cpu_write(current_kprobe, p);
		kcb->kprobe_status = KPROBE_HIT_ACTIVE;
		if (!p->pre_handler || !p->pre_handler(p, regs)) {
			__skip_singlestep(p, regs, kcb);
			preempt_enable_no_resched();
		}
		/*
		 * If pre_handler returns !0, it sets regs->ip and
		 * resets current kprobe, and keep preempt count +1.
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
		 */
	}
end:
	local_irq_restore(flags);
}
NOKPROBE_SYMBOL(kprobe_ftrace_handler);

int arch_prepare_kprobe_ftrace(struct kprobe *p)
{
	p->ainsn.insn = NULL;
	p->ainsn.boostable = -1;
	return 0;
}
