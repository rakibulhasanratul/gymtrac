#ifndef GYMTRAC_RNG_H
#define GYMTRAC_RNG_H

/**
 * Seeds the xoshiro128** pseudorandom number generator.
 *
 * Seed mixing keeps the internal state from being all zero, as the
 * algorithm requires.
 *
 * @param seed the seed value
 */
void seed_rng(unsigned int seed);

/**
 * Returns the next pseudorandom unsigned int from the xoshiro128** generator.
 *
 * @return a pseudorandom unsigned int
 */
unsigned int random_number(void);

#endif
