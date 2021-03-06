#ifndef _ASM_X86_DISABLED_FEATURES_H
#define _ASM_X86_DISABLED_FEATURES_H

/* These features, although they might be available in a CPU
 * will not be used because the compile options to support
 * them are not present.
 *
 * This code allows them to be checked and disabled at
 * compile time without an explicit #ifdef.  Use
 * cpu_feature_enabled().
 */

#ifdef CONFIG_X86_64
# define DISABLE_VME		(1<<(X86_FEATURE_VME & 31))
# define DISABLE_K6_MTRR	(1<<(X86_FEATURE_K6_MTRR & 31))
# define DISABLE_CYRIX_ARR	(1<<(X86_FEATURE_CYRIX_ARR & 31))
# define DISABLE_CENTAUR_MCR	(1<<(X86_FEATURE_CENTAUR_MCR & 31))
<<<<<<< HEAD
=======
# define DISABLE_PCID		0
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#else
# define DISABLE_VME		0
# define DISABLE_K6_MTRR	0
# define DISABLE_CYRIX_ARR	0
# define DISABLE_CENTAUR_MCR	0
<<<<<<< HEAD
=======
# define DISABLE_PCID		(1<<(X86_FEATURE_PCID & 31))
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#endif /* CONFIG_X86_64 */

/*
 * Make sure to add features to the correct mask
 */
#define DISABLED_MASK0	(DISABLE_VME)
#define DISABLED_MASK1	0
#define DISABLED_MASK2	0
#define DISABLED_MASK3	(DISABLE_CYRIX_ARR|DISABLE_CENTAUR_MCR|DISABLE_K6_MTRR)
<<<<<<< HEAD
#define DISABLED_MASK4	0
=======
#define DISABLED_MASK4	(DISABLE_PCID)
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#define DISABLED_MASK5	0
#define DISABLED_MASK6	0
#define DISABLED_MASK7	0
#define DISABLED_MASK8	0
#define DISABLED_MASK9	0

#endif /* _ASM_X86_DISABLED_FEATURES_H */
