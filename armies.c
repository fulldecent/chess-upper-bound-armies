/*
 * Finds all possible armies for one player, and [ASP] for each
 */

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
	int base_army[5] = {0,1,2,2,2}; /* Armies attainable without pawns or promotion */
	int i;
	
	for(army[0]=0; army[0]<=8; army[0]++)
	for(army[1]=0; army[1]<=9; army[1]++)
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

		p = 1;
		for (i=0; i<5; i++)
			p *= fact(army[i]);
		
/*		printf("ARMY[]=%d%d%d%d%d -- ", army[0],army[1],army[2],army[3],army[4]);*/
		printf("%2d\t%3d\t%9d\tK", army[0], s, p);
		for(i=0; i<army[0]; i++)
			putchar('P');
		for(i=0; i<army[1]; i++)
			putchar('Q');
		for(i=0; i<army[2]; i++)
			putchar('R');
		for(i=0; i<army[3]; i++)
			putchar('B');
		for(i=0; i<army[4]; i++)
			putchar('N');
		puts("");
	}
	
	return 0;
}
