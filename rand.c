/*
 * Random numbers between 0 and 22124621884617108585387385940828998876019391611
 * inclusive
 */

#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>
#include <string.h> /* for memcmp */
#include "fen.h"

int main()
{
	mpz_t r, max;
	gmp_randstate_t gmpRandState;

	mpz_init(r);
	mpz_init_set_str(max, "22124621884617108585387385940828998876019391612", 10);
	gmp_randinit_default(gmpRandState);
	gmp_randseed_ui(gmpRandState, 1234567890);

	for (;;) {
		mpz_urandomm(r, gmpRandState, max);
		gmp_printf("%Zd\n", r);
	}
	return 0;
	
}
