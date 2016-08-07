/*
 * An upper bound on the number of possible chess positions.
 * See the file: entriken
 *
 * Some illegal positions will not be encoded
 */
#error THIS FILE GETS UGLIER EVERY TIME I LOOK AT IT
#error This file attempts to handle all cases of enpassent and castling
#error progress: aside from king placement, this is close
#error However, this is only for studying, not running


#warning DONT FORGET side to act

#include "hashtable.h"
#include "hashtable_itr.h"
#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>
#include <string.h> /* for memcmp */


enum PIECES { space=0, wk, wp, wq, wr, wb, wn, bk, bp, bq, br, bb, bn, NUMPIECES}; 

/* Speedup: hash table permutation calculations */
struct key { int aa; int ss; int64_t pp; int s64; int s48; int k1; int k2; };
struct value {mpz_t * combs;};
DEFINE_HASHTABLE_INSERT(insert_some, struct key, struct value);
DEFINE_HASHTABLE_SEARCH(search_some, struct key, struct value);

static unsigned int hashfromkey(void *ky)
{
	struct key *k = (struct key *)ky;
	return ((k->aa<<28) + (k->ss<<24)) + (k->s64<<8) + (k->s48<<2) + (k->pp>>32) + (k->pp) + (k->k1<<10) + (k->k2<<15);
}

static int equalkeys(void *k1, void *k2)
{
	return (0 == memcmp(k1,k2,sizeof(struct key)));
}



int main()
{	
	int enpassant, casK, casQ, cask, casq;
	int army[5]={0};                /* PAWNS, QUEENS, ROOKS, BISHOPS, KNIGHTS*/
	int b;                          /* Number promoted pieces */
	int abc;                        /* Army size, less kings and fixed pieces */
	int p;                          /* Product */
	int ARMY[5]={0};       
	int B;                 
	int ABC;                 
	int P;                 

	int pawncap, allcap, PAWNCAP, ALLCAP; /* To check if two armies can occur together */
	int i;

	struct key *k;
	struct value *v, *found;
	struct hashtable *h;

	int fact[11] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800};
	mpz_t facts[65];                /* precompute fact(0..64) */
	mpz_t total, totallast;   
	mpz_t current, tmp;

	k = malloc(sizeof(struct key)); /* saves relevant parts of perm calculation */

	mpz_init(total);
	mpz_init(totallast);
	mpz_init(current);
	mpz_init(tmp);

	for (i=0; i<=64; i++) {
		mpz_init(facts[i]);
		mpz_fac_ui(facts[i], i);
	}

	/* "ROUNDS" */
	for(enpassant=0; enpassant<=8; enpassant++)
	for(casK=0; casK<=1; casK++)
	for(casQ=0; casQ<=1; casQ++)
	for(cask=0; cask<=1; cask++)
	for(casq=0; casq<=1; casq++)
	{
		h = create_hashtable(16, hashfromkey, equalkeys);
		if (NULL == h) exit(-1); /*oom*/
/* do we need to flush this each round? */

		mpz_t subtotal;
		mpz_init(subtotal);
		printf(">> %i %i %i %i %i\n", enpassant, casK, casQ, cask, casq);

#warning SWITCH CASES
	/* First handle the army to act */
	for(army[0]=0; army[0]<=8-(enpassant>0?1:0); army[0]++)
	for(army[1]=0; army[1]<=9; army[1]++)
	for(army[2]=0; army[2]<=10-casK-casQ; army[2]++)
	for(army[3]=0; army[3]<=10; army[3]++)
	for(army[4]=0; army[4]<=10; army[4]++)
	{
		abc = army[0]+army[1]+army[2]+army[3]+army[4];
		if (abc>15)
			continue; /* Army too big */

		/* minimum number of promotions to reach this army */
		b = army[1]>1 ? army[1]-1 : 0;
		if (army[2]>2-casK-casQ) b += army[2] - (2-casK-casQ);
		if (army[3]>2) b += army[3]-2;
		if (army[4]>2) b += army[4]-2;
		if (army[0]+b > 8-(enpassant>0?1:0))
			continue; /* Impossible promotions */

		p = fact[army[0]]*fact[army[1]]*fact[army[2]]*fact[army[3]]*fact[army[4]];

		for(ARMY[0]=0; ARMY[0]<=8-(enpassant>0?1:0); ARMY[0]++)
		for(ARMY[1]=0; ARMY[1]<=9; ARMY[1]++)
		for(ARMY[2]=0; ARMY[2]<=10-cask-casq; ARMY[2]++)
		for(ARMY[3]=0; ARMY[3]<=10; ARMY[3]++)
		for(ARMY[4]=0; ARMY[4]<=10; ARMY[4]++)
		{

			ABC = ARMY[0]+ARMY[1]+ARMY[2]+ARMY[3]+ARMY[4];
			if (ABC>15)
				continue; /* Army too big */
	
			B = ARMY[1]>1 ? ARMY[1]-1 : 0;
			if (ARMY[2]>2-cask-casq) B += ARMY[2]- (2-cask-casq);
			if (ARMY[3]>2) B += ARMY[3]-2;
			if (ARMY[4]>2) B += ARMY[4]-2;
			if (ARMY[0]+B > 8-(enpassant>0?1:0))
				continue; /* Impossible promotions */
	
			/* Can these two armies occur together? */
			allcap = 30-2*(enpassant>0?1:0)-casK-casQ-cask-casq-abc-ABC;
			if ((b>allcap+8-(enpassant>0?1:0)-ARMY[0]-B) || (B>allcap+8-(enpassant>0?1:0)-army[0]-b))
				continue;

			P = fact[ARMY[0]]*fact[ARMY[1]]*fact[ARMY[2]]*fact[ARMY[3]]*fact[ARMY[4]];

			/*** Below here, we calculate all permutations, which is cached,
			     these are based on pieces that aren't fixed. ***/
		
			k->aa = army[0] + ARMY[0];
			k->ss = abc+ABC+2;
#warn fix this -> kings
			k->pp = (int64_t)p*P;
			/* Are either kings restricted? */
			k->k1 = casK || casQ;
			k->k2 = cask || casq;
			/* The number of squares that have not been committed */
			/* King committment is handled below, enpassant committment
	                   is weird for file B through G, handled below. */
			k->s64 = 64 - (enpassant>1?4:0)-casK-casQ-cask-casq;
			/* The number of squares for pawns not yet committed */
			k->s48 = 48 - (enpassant>1?4:0);



			if ((found = search_some(h,k)))
			{
				mpz_add(total, total, *(((struct value *)found)->combs));
			}
			else
			{
/* COUNT FOR DIFFERENT KING PLACEMENTS */
/* COUNT FOR EP PLACEMENT ON LEFT OR RIGHT IF B..G */
  /* count twice and subtract? */
/* CASTLE KING IS FIXED */

				mpz_set_ui(current, 1);
				mpz_set_ui(tmp, 1);

				mpz_set_ui(current, 1);
				mpz_mul(current, current, facts[k->s48]);
				mpz_divexact(current, current, facts[k->s48 - k->aa]);
				mpz_mul(current, current, facts[k->s64 - k->aa]);
				mpz_divexact(current, current, facts[k->s64 - k->ss]);
				mpz_divexact_ui(current, current, P);
				mpz_divexact_ui(current, current, p);

				v = malloc(sizeof(struct value));
				if (NULL == v) exit(-1); /*oom*/
				v->combs = malloc(sizeof(mpz_t));
				if (NULL == v->combs) exit(-1); /*oom*/
				mpz_init(*(v->combs));
				mpz_set(*(v->combs), current);
				if (!insert_some(h,k,v)) exit(-1); /*oom*/
				k = (struct key *)malloc(sizeof(struct key));
				if (NULL == k) exit(-1); /*oom*/

				mpz_add(total, total, current);
			}



		}
	}
		gmp_printf("round total: %Zd\n", total);

#warning DONT NEED THIS?
		hashtable_destroy(h, 1);

	} /* END "ROUNDS" */

	mpz_out_str(stdout, 10, total);
	return 0;
}


void init_cache()
{

}
/*
struct position;

void position_to_cpi(const struct position * p, mpz_t * cpi)
{
	int i;
	int armies[NUMPIECES];
	
	for (i=0, i<65; i++)
		armies[p->diagram[i]]++;
	 
	mpz_set(cpi, cpi );
}

void diagram_to_cpi(const position * p, mpz_t * cpi)
{

}
*/
