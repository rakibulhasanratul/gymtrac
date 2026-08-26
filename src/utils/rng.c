// xoshiro128** pseudorandom number generator.
//
// Non-cryptographic PRNG by David Blackman and Sebastiano Vigna; NOT suitable
// for cryptographic purposes.
//
// Reference: https://prng.di.unimi.it/xoshiro128starstar.c
//
// Bitwise operators (<<, >>, ^, |) are avoided; power_of_2_to_unsigned_int(),
// xor_32(), and rotate_left_32() compute the same results arithmetically.

#include <math.h>
#include <time.h>

#include "rng.h"

// 128-bit state: four 32-bit words.
static unsigned int state[4];

// Returns 2^n as an unsigned int. Valid for n in [0, 31].
static inline unsigned int power_of_2_to_unsigned_int(unsigned int n)
{
  return (unsigned int)pow(2.0, (double)n);
}

// Computes a XOR b using repeated modulo to extract bits.
static inline unsigned int xor_32(unsigned int a, unsigned int b)
{
  unsigned int result = 0;
  unsigned int place = 1;
  while (a > 0 || b > 0)
  {
    unsigned int bit_a = a % 2;
    unsigned int bit_b = b % 2;
    if (bit_a != bit_b) result += place;
    a /= 2;
    b /= 2;
    place *= 2;
  }
  return result;
}

// Left rotation of a 32-bit value: (x << k) | (x >> (32 - k)).
static inline unsigned int rotate_left_32(unsigned int x, unsigned int k)
{
  return x * power_of_2_to_unsigned_int(k) + x / power_of_2_to_unsigned_int(32 - k);
}

// Advances the xoshiro128** state by one step.
static void xoshiro128_next(void)
{
  unsigned int t = state[1] * power_of_2_to_unsigned_int(9);

  state[2] = xor_32(state[2], state[0]);
  state[3] = xor_32(state[3], state[1]);
  state[1] = xor_32(state[1], state[2]);
  state[0] = xor_32(state[0], state[3]);

  state[2] = xor_32(state[2], t);

  state[3] = rotate_left_32(state[3], 11);
}

void seed_rng(unsigned int seed)
{
  unsigned int mixed = seed + 2654435769;
  mixed = mixed * 2654435761 + 3266489917;

  state[0] = mixed;
  state[1] = mixed + power_of_2_to_unsigned_int(8);
  state[2] = mixed + power_of_2_to_unsigned_int(16);
  state[3] = mixed + power_of_2_to_unsigned_int(24);
}

unsigned int random_number(void)
{
  xoshiro128_next();
  return rotate_left_32(state[1] * 5, 7) * 9;
}
