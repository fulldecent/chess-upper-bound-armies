/*
 * An upper bound on the number of possible chess diagrams.
 *
 * This ignores king placement and does not have any speedups.
 */

#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>
#include <string.h> /* for memcmp */


int main()
{	
	int army[5]={0};                /* PAWNS, QUEENS, ROOKS, BISHOPS, KNIGHTS*/
	int b;                          /* Number promoted pieces */
	int s;                          /* Total army size (1 + a + b + c) */
	int p;                          /* Product */
	int ARMY[5]={0};       
	int B;                 
	int S;                 
	int P;                 

	int a2bc, _38_2a2bc;            /* Comparitors for A+2B+C <= 38-c-2a */
	int i;

	int fact[11] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800};
	mpz_t facts[65];                /* precompute fact(0..64) */
	mpz_t total;   
	mpz_t current;

	mpz_init(total);
	mpz_init(current);

	for (i=0; i<=64; i++) {
		mpz_init(facts[i]);
		mpz_fac_ui(facts[i], i);
	}

mpz_t temp, temp2;
mpz_init(temp);
mpz_init(temp2);
	for(army[0]=0; army[0]<=8; army[0]++)
	for(army[1]=0; army[1]<=9; army[1]++)
	for(army[2]=0; army[2]<=10; army[2]++)
	for(army[3]=0; army[3]<=10; army[3]++)
	for(army[4]=0; army[4]<=10; army[4]++)
	{
		s = 1+army[0]+army[1]+army[2]+army[3]+army[4];
		if (s>16)
			continue; /* Army too big */

		b = army[1]>1 ? army[1]-1 : 0;
		if (army[2]>2) b += army[2]-2;
		if (army[3]>2) b += army[3]-2;
		if (army[4]>2) b += army[4]-2;
		if (army[0]+b > 8)
			continue; /* Impossible promotions */

		p = fact[army[0]]*fact[army[1]]*fact[army[2]]*fact[army[3]]*fact[army[4]];

		a2bc = s-1 + b;
		_38_2a2bc = 38 - (s-1 + army[0] + b);

		for(ARMY[0]=0; ARMY[0]<=8; ARMY[0]++)
		for(ARMY[1]=0; ARMY[1]<=9; ARMY[1]++)
		for(ARMY[2]=0; ARMY[2]<=10; ARMY[2]++)
		for(ARMY[3]=0; ARMY[3]<=10; ARMY[3]++)
		for(ARMY[4]=0; ARMY[4]<=10; ARMY[4]++)
		{
			S = 1+ARMY[0]+ARMY[1]+ARMY[2]+ARMY[3]+ARMY[4];
			if (S>16)
				continue; /* Army too big */
	
			B = ARMY[1]>1 ? ARMY[1]-1 : 0;
			if (ARMY[2]>2) B += ARMY[2]-2;
			if (ARMY[3]>2) B += ARMY[3]-2;
			if (ARMY[4]>2) B += ARMY[4]-2;
			if (ARMY[0]+B > 8)
				continue; /* Impossible promotions */
	
			/* Is either army combination rule broken? */
			if ((a2bc > (38 - (S-1 + ARMY[0] + B))) || ((S-1 + B) > _38_2a2bc))
				continue;

			P = fact[ARMY[0]]*fact[ARMY[1]]*fact[ARMY[2]]*fact[ARMY[3]]*fact[ARMY[4]];

			/* For the given white and black armies, there are:
			   48! * (64-A-a)! / (48-A-a)! / (64-S-s)! / P / p
			   possible placements
          	 	*/
			
			mpz_set_ui(current, 1);
			mpz_mul(current, current, facts[48]);
			mpz_divexact(current, current, facts[48 - army[0] - ARMY[0]]);
			mpz_mul(current, current, facts[64 - army[0] - ARMY[0]]);
			mpz_divexact(current, current, facts[64 - S - s]);
			mpz_divexact_ui(current, current, P);
			mpz_divexact_ui(current, current, p);
			mpz_add(total, total, current);
		}
	}

mpz_sub(temp2, total, temp);
mpz_add(temp, temp, temp2);
mpz_out_str(stdout, 10, temp2);
puts("");

	mpz_out_str(stdout, 10, total);
	puts("");
	return 0;
}

