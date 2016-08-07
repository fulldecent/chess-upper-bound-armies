/*
 * Finds all possible armies for both player, and some statistics about them.
 * Takes into consideration limits on individual armies
 * and limits on both players' armies at the same time.
 *
 * EG: you cannot promote, unless there is a capture
 *
 * WARNING:
 *   Using PRINT_ARMY_PRODUCTS below outputs 800MB, 
 *   and has runtime 10min on 2009 MacBook Pro
 */
#define PRINT_ARMY_PRODUCTS

#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>

int fact (int i)
{
	return (i<=1) ? 1 : i*fact(i-1);
}

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
	int64_t pP;

	int base_army[5] = {0,1,2,2,2}; /* Armies attainable without pawns or promotion */
	int a2bc, _38_2a2bc;            /* Comparitors for A+2B+C <= 38-2a-2b-c */
	int i;
	int count = 0;

	for(army[0]=0; army[0]<=8; army[0]++)
	for(army[1]=0; army[1]<=9; army[1]++)
{ /* move this up or down to change verbosity */
	for(army[2]=0; army[2]<=10; army[2]++)
	for(army[3]=0; army[3]<=10; army[3]++)
	for(army[4]=0; army[4]<=10; army[4]++)
	{
		s = 1+army[0]+army[1]+army[2]+army[3]+army[4];
		if (s>16)
			continue; /* Quick rule-out check */

		b = 0;
		for (i=1; i<5; i++)
			if (army[i] > base_army[i]) 
				b += army[i] - base_army[i];
		if (army[0] + b > 8)
			continue; /* Impossible army */
	
		/* p is not used in this program */
		p = 1;
		for (i=0; i<5; i++)
			p *= fact(army[i]);
	
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
				continue; /* Quick rule-out check */

			B = 0;
			for (i=1; i<5; i++)
				if (ARMY[i] > base_army[i]) 
					B += ARMY[i] - base_army[i];
			if (ARMY[0] + B > 8)
				continue; /* Impossible army */

#ifdef PRINT_ARMY_PRODUCTS
			/* P is not used in this program */
			P = 1;
			for (i=0; i<5; i++)
				P *= fact(ARMY[i]);

			/* Is either army combination rule broken? */
			if ((a2bc > (38 - (S-1 + ARMY[0] + B))) || ((S-1 + B) > _38_2a2bc))
				continue;

			pP = p;
			pP *= P;
			printf("%2d\t%2d\t%lld\n", army[0]+ARMY[0], s+S, pP);
#endif
			count++;
		}
	}
fprintf(stderr, "%d\n", count);
}
	return 0;
}
