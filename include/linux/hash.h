#ifndef _LINUX_HASH_H
#define _LINUX_HASH_H
/* Fast hashing routine for ints,  longs and pointers.
   (C) 2002 Nadia Yvette Chambers, IBM */

/*
 * Knuth recommends primes in approximately golden ratio to the maximum
 * integer representable by a machine word for multiplicative hashing.
 * Chuck Lever verified the effectiveness of this technique:
 * http://www.citi.umich.edu/techreports/reports/citi-tr-00-1.pdf
 *
 * These primes are chosen to be bit-sparse, that is operations on
 * them can use shifts and additions instead of multiplications for
 * machines where multiplications are slow.
 */

#include <asm/types.h>
#include <asm/hash.h>
#include <linux/compiler.h>

/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 0x9e370001UL
/*  2^63 + 2^61 - 2^57 + 2^54 - 2^51 - 2^18 + 1 */
#define GOLDEN_RATIO_PRIME_64 0x9e37fffffffc0001UL

#if BITS_PER_LONG == 32
#define GOLDEN_RATIO_PRIME GOLDEN_RATIO_PRIME_32
#define hash_long(val, bits) hash_32(val, bits)
#elif BITS_PER_LONG == 64
#define hash_long(val, bits) hash_64(val, bits)
#define GOLDEN_RATIO_PRIME GOLDEN_RATIO_PRIME_64
#else
#error Wordsize not 32 or 64
#endif

<<<<<<< HEAD
=======
/*
 * The above primes are actively bad for hashing, since they are
 * too sparse. The 32-bit one is mostly ok, the 64-bit one causes
 * real problems. Besides, the "prime" part is pointless for the
 * multiplicative hash.
 *
 * Although a random odd number will do, it turns out that the golden
 * ratio phi = (sqrt(5)-1)/2, or its negative, has particularly nice
 * properties.
 *
 * These are the negative, (1 - phi) = (phi^2) = (3 - sqrt(5))/2.
 * (See Knuth vol 3, section 6.4, exercise 9.)
 */
#define GOLDEN_RATIO_32 0x61C88647
#define GOLDEN_RATIO_64 0x61C8864680B583EBull

>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
static __always_inline u64 hash_64(u64 val, unsigned int bits)
{
	u64 hash = val;

<<<<<<< HEAD
#if defined(CONFIG_ARCH_HAS_FAST_MULTIPLIER) && BITS_PER_LONG == 64
	hash = hash * GOLDEN_RATIO_PRIME_64;
=======
#if BITS_PER_LONG == 64
	hash = hash * GOLDEN_RATIO_64;
>>>>>>> 21c1bccd7c23ac9673b3f0dd0f8b4f78331b3916
#else
	/*  Sigh, gcc can't optimise this alone like it does for 32 bits. */
	u64 n = hash;
	n <<= 18;
	hash -= n;
	n <<= 33;
	hash -= n;
	n <<= 3;
	hash += n;
	n <<= 3;
	hash -= n;
	n <<= 4;
	hash += n;
	n <<= 2;
	hash += n;
#endif

	/* High bits are more random, so use them. */
	return hash >> (64 - bits);
}

static inline u32 hash_32(u32 val, unsigned int bits)
{
	/* On some cpus multiply is faster, on others gcc will do shifts */
	u32 hash = val * GOLDEN_RATIO_PRIME_32;

	/* High bits are more random, so use them. */
	return hash >> (32 - bits);
}

static inline unsigned long hash_ptr(const void *ptr, unsigned int bits)
{
	return hash_long((unsigned long)ptr, bits);
}

static inline u32 hash32_ptr(const void *ptr)
{
	unsigned long val = (unsigned long)ptr;

#if BITS_PER_LONG == 64
	val ^= (val >> 32);
#endif
	return (u32)val;
}

struct fast_hash_ops {
	u32 (*hash)(const void *data, u32 len, u32 seed);
	u32 (*hash2)(const u32 *data, u32 len, u32 seed);
};

/**
 *	arch_fast_hash - Caclulates a hash over a given buffer that can have
 *			 arbitrary size. This function will eventually use an
 *			 architecture-optimized hashing implementation if
 *			 available, and trades off distribution for speed.
 *
 *	@data: buffer to hash
 *	@len: length of buffer in bytes
 *	@seed: start seed
 *
 *	Returns 32bit hash.
 */
extern u32 arch_fast_hash(const void *data, u32 len, u32 seed);

/**
 *	arch_fast_hash2 - Caclulates a hash over a given buffer that has a
 *			  size that is of a multiple of 32bit words. This
 *			  function will eventually use an architecture-
 *			  optimized hashing implementation if available,
 *			  and trades off distribution for speed.
 *
 *	@data: buffer to hash (must be 32bit padded)
 *	@len: number of 32bit words
 *	@seed: start seed
 *
 *	Returns 32bit hash.
 */
extern u32 arch_fast_hash2(const u32 *data, u32 len, u32 seed);

#endif /* _LINUX_HASH_H */
