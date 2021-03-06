/* timex.h: FR-V architecture timex specifications
 */
#ifndef _ASM_TIMEX_H
#define _ASM_TIMEX_H

#define CLOCK_TICK_RATE		1193180 /* Underlying HZ */
#define CLOCK_TICK_FACTOR	20	/* Factor of both 1000000 and CLOCK_TICK_RATE */

typedef unsigned long cycles_t;

static inline cycles_t get_cycles(void)
{
	return 0;
}

#define vxtime_lock()		do {} while (0)
#define vxtime_unlock()		do {} while (0)

<<<<<<< HEAD
=======
/* This attribute is used in include/linux/jiffies.h alongside with
 * __cacheline_aligned_in_smp. It is assumed that __cacheline_aligned_in_smp
 * for frv does not contain another section specification.
 */
#define __jiffy_arch_data	__attribute__((__section__(".data")))

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#endif

