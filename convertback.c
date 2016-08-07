/*
 * Converts CPI to FEN based on an ordering given in README
 * ONLY ENCODES THE DIAGRAM
 *
 * This program works for positions with
 *   No enpassant
 *   No castling available
 */

#include "hashtable.h"
#include "hashtable_itr.h"
#include <stdlib.h>
#include <stdio.h>
#include <gmp.h>
#include <string.h> /* for memcmp */
#include "fen.h"

//#define DEBUG /* allows testing of assumptions in code */

int abs (int i) { return i < 0 ? -i : i; }
enum PIECETYPES { wk=0, bk, wp, wq, wr, wb, wn, bp, bq, br, bb, bn, empty, ignore, NUMPIECETYPES}; 
mpz_t facts[65];                /* precompute fact(0..64) */
int fact[11] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800};

struct position {
    int diagram[64];   /* rank 8 to 1, file A to H */
    int color;         /* w=0, b=1 */
    int castling[4];   /* KQkq, no=0, yes=1 */
    char enpassant;    /* '-' for none, 'a'..'h' */
};

/* Speedup: hash table permutation calculations */
struct key { int allspace, pspace, numpawns, numempty; int64_t pieceperms; };
struct value {mpz_t * combs;};
DEFINE_HASHTABLE_INSERT(insert_some, struct key, struct value);
DEFINE_HASHTABLE_SEARCH(search_some, struct key, struct value);

static unsigned int hashfromkey(void *ky)
{
	struct key *k = (struct key *)ky;
	return (k->pieceperms)+(k->pieceperms>>32)+(k->allspace<<28)+(k->pspace<<24)+(k->numpawns<<10)+(k->numempty<<2);
}

static int equalkeys(void *k1, void *k2)
{
	return (0 == memcmp(k1,k2,sizeof(struct key)));
}


int write_kings(mpz_t *cpi, int diagram[64], int *wkp, int *bkp);
int write_armies(mpz_t *cpi, int armies[NUMPIECETYPES], int ek);
int write_permutation(mpz_t *cpi, int diagram[64], int armies[NUMPIECETYPES]);
int write_diagram(mpz_t *cpi, int diagram[64]);


/* Subtracts from CPI the number of positions up to ones with kings placed as in POSITION
   King positions are saved to WKP, BKP
   Returns 0 if king placement is invalid 
*/
int write_kings(mpz_t *cpi, int diagram[64], int *wkp, int *bkp)
{
	int i, j;
	mpz_t king_both_edge, king_one_edge, king_neither_edge;

#ifdef DEBUG 
#error You need to test these values from convert.c
#else
	mpz_init_set_str(king_both_edge, "7394384359052019860110129861127614655263725", 10);
	mpz_init_set_str(king_one_edge, "6499245207374987910384446769196893420982287", 10);
	mpz_init_set_str(king_neither_edge, "5710094948882734570085737442865185909590743", 10);
#endif

	/* Traverse all possible king positions in README FILE order */
	for (i=0; i<64; i++) {
		for (j=0; j<64; j++) {
			if (abs(i%8-j%8)<=1 && abs(i/8-j/8)<=1)
				continue; /* touching kings */
			if ((i/8)%7==0 && (j/8)%7==0) {
				if (mpz_cmpabs(king_both_edge, *cpi) > 0) {
					*wkp = i; *bkp = j;
					diagram[i] = wk; diagram[j] = bk;
					return 1;
				}
				mpz_sub(*cpi, *cpi, king_both_edge);
			}
			else if ((i/8)%7==0 || (j/8)%7==0) {
				if (mpz_cmpabs(king_one_edge, *cpi) > 0) {
					*wkp = i; *bkp = j;
					diagram[i] = wk; diagram[j] = bk;
					return 1;
				}
				mpz_sub(*cpi, *cpi, king_one_edge);
			}
			else {
				if (mpz_cmpabs(king_neither_edge, *cpi) > 0) {
					*wkp = i; *bkp = j;
					diagram[i] = wk; diagram[j] = bk;
					return 1;
				}
				mpz_sub(*cpi, *cpi, king_neither_edge);
			}
		}
	}

	printf("Error: failed to read kings\n");	
	return 0; /* kings in POSITON are not one of the valid ones traversed above */
}

/* Subtracts from RETVAL the number of positions up to ones with armies as in POSITION, where:
   -- King placement is given
   -- EK number of edge kings
   Returns 0 if armies are invalid 
   If ALL is non zero, ignore POSITIONS and count for all possible armies
*/
int write_armies(mpz_t *cpi, int armies[NUMPIECETYPES], int ek)
{
	mpz_t current;
	int B, ABC, P; 	  /* Parameters for white */
	int b, abc, p;    /* Parameters for black */
	int allcap;
	struct key *k;
	struct value *v, *found;
	struct hashtable *h;

	mpz_init(current);

	h = create_hashtable(16, hashfromkey, equalkeys);
	if (NULL == h) exit(-1); /*oom*/	
	k = malloc(sizeof(struct key)); /* saves relevant parts of perm calculation */
	if (NULL == k) exit(-1); /*oom*/	
	
	/* First handle the white army */
	for(armies[wp]=0; armies[wp]<=8; armies[wp]++)
	for(armies[wq]=0; armies[wq]<=9; armies[wq]++)
	for(armies[wr]=0; armies[wr]<=10; armies[wr]++)
	for(armies[wb]=0; armies[wb]<=10; armies[wb]++)
	for(armies[wn]=0; armies[wn]<=10; armies[wn]++)
	{
		ABC = armies[wp]+armies[wq]+armies[wr]+armies[wb]+armies[wn];
		if (ABC>15)
			continue; /* army too big */

		/* Minimum number of promotions to reach this army */
		B = armies[wq]>1 ? armies[wq]-1 : 0;
		if (armies[wr]>2) B += armies[wr]-2;
		if (armies[wb]>2) B += armies[wb]-2;
		if (armies[wn]>2) B += armies[wn]-2;
		if (armies[wp]+B > 8)
			continue; /* too many promotions, given pawns */

		P = fact[armies[wp]]*fact[armies[wq]]*fact[armies[wr]]*fact[armies[wb]]*fact[armies[wn]];

		for(armies[bp]=0; armies[bp]<=8; armies[bp]++)
		for(armies[bq]=0; armies[bq]<=9; armies[bq]++)
		for(armies[br]=0; armies[br]<=10; armies[br]++)
		for(armies[bb]=0; armies[bb]<=10; armies[bb]++)
		for(armies[bn]=0; armies[bn]<=10; armies[bn]++)
		{
			abc = armies[bp]+armies[bq]+armies[br]+armies[bn]+armies[bb];
			if (abc>15)
				continue; /* army too big */
	
			b = armies[bq]>1 ? armies[bq]-1 : 0;
			if (armies[br]>2) b += armies[br]-2;
			if (armies[bb]>2) b += armies[bb]-2;
			if (armies[bn]>2) b += armies[bn]-2;
			if (armies[bp]+b > 8)
				continue; /* too many promotions, given pawns */
	
			/* Can these two armies occur together? */
			allcap = 30-ABC-abc;
			if ((b>allcap+8-armies[wp]-B) || (B>allcap+8-armies[bp]-b))
				continue;

			p = fact[armies[bp]]*fact[armies[bq]]*fact[armies[br]]*fact[armies[bb]]*fact[armies[bn]];
		

			k->allspace = 62;
			k->pspace = 46+ek;
			k->numpawns = armies[wp] + armies[bp];
			k->numempty = 62 - ABC - abc;
			k->pieceperms = (int64_t)P*p;
			
			if ((found = search_some(h,k)))
			{
				if (mpz_cmpabs(*(((struct value *)found)->combs), *cpi) > 0) {
					hashtable_destroy(h, 1);
					return 1;
				}
				mpz_sub(*cpi, *cpi, *(((struct value *)found)->combs));
			}
			else
			{
				mpz_set(current, facts[k->pspace]);
				mpz_divexact(current, current, facts[k->pspace - k->numpawns]);
				mpz_mul(current, current, facts[k->allspace - k->numpawns]);
				mpz_divexact(current, current, facts[k->numempty]);
				mpz_divexact_ui(current, current, P);
				mpz_divexact_ui(current, current, p);

				if (mpz_cmpabs(current, *cpi) > 0) {
					hashtable_destroy(h, 1);
					return 1;
				}
				mpz_sub(*cpi, *cpi, current);

				v = malloc(sizeof(struct value));
				if (NULL == v) exit(-1); /*oom*/
				v->combs = malloc(sizeof(mpz_t));
				if (NULL == v->combs) exit(-1); /*oom*/
				mpz_init_set(*(v->combs), current);
				if (!insert_some(h,k,v)) exit(-1); /*oom*/
				k = (struct key *)malloc(sizeof(struct key));
				if (NULL == k) exit(-1); /*oom*/
			}
		}
	}
	
	hashtable_destroy(h, 1);
	return 0;
}

/* Adds to RETVAL the number of permutations of armies up to the one as in POSITION.
   Ignores pieces that are IGNORE
   Returns 0 if permutation is invalid
*/
int write_permutation(mpz_t *cpi, int diagram[64], int armies[NUMPIECETYPES])
{
	int i, j, k, allspace, pspace;
	mpz_t current;

	mpz_init(current);

	for (i=0; i<64; i++) {
		if (diagram[i] == ignore)
			continue;
		for (j=wp; j<ignore; j++) {
			if ((j==wp || j==bp) && (i<8 || i>55))
				continue; /* a pawn can't go here */
			if (armies[j] <= 0)
				continue; /* no more of this piece left */

			armies[j]--;

			/* Case: How many ways can we place remaining ARMIES on squares past K? */
			allspace = pspace = 0;
			for (k=i+1; k<64; k++) {
				if (diagram[k] == ignore)
					continue;
				if (k>=8 && k<=55)
					pspace++;
				allspace++;
			}

			mpz_set(current, facts[pspace]);
			mpz_divexact(current, current, facts[pspace - armies[wp] - armies[bp]]);
			mpz_mul(current, current, facts[allspace - armies[wp] - armies[bp]]);
			mpz_divexact(current, current, facts[armies[empty]]); // empty squares
			mpz_divexact_ui(current, current, fact[armies[wp]]*fact[armies[wq]]*fact[armies[wr]]*fact[armies[wb]]*fact[armies[wn]]);
			mpz_divexact_ui(current, current, fact[armies[bp]]*fact[armies[bq]]*fact[armies[br]]*fact[armies[bb]]*fact[armies[bn]]);

			if (mpz_cmpabs(current, *cpi) > 0) {
				diagram[i]=j;
				break;
			}
			mpz_sub(*cpi, *cpi, current);

			/* End Case */
			armies[j]++;
		}
		gmp_printf(" -- CPI w/ diagram[%2d]: %Zd\n", i, *cpi);

	}
	return 1;
}

/* 1 on success, 0 on invalid input */
int write_diagram(mpz_t *cpi, int diagram[64])
{
	int wkp, bkp;  /* king positions */
	int ek; /* kings in edge position (1&8 rank) */
	int i, j=0;
	int armies[NUMPIECETYPES] = {0};

	for (i=0; i<64; i++)
		diagram[i] = empty;

	i = write_kings(cpi, diagram, &wkp, &bkp);
	if (!i) return 0;
//	gmp_printf(" -- CPI without kings: %Zd\n", *cpi);

	/* How many edge kings are there? */
	if ((wkp/8)%7==0 && (bkp/8)%7==0)
		ek=2;
	else if ((wkp/8)%7==0 || (bkp/8)%7==0)
		ek=1;
	else
		ek=0;

	diagram[wkp] = ignore;
	diagram[bkp] = ignore;
	armies[ignore] = 2;

	i = write_armies(cpi, armies, ek);
	if (!i) return 0;
	for (i=0; i<NUMPIECETYPES; i++)
		j += armies[i];
	armies[empty] = 64-j;
//	gmp_printf(" -- CPI without armies: %Zd\n", *cpi);


	i = write_permutation(cpi, diagram, armies);
	if (!i) return 0;
	diagram[wkp] = wk;
	diagram[bkp] = bk;

	return 1;
}


int main()
{
	char fen[1024] = { 0 };
	struct position p;
	int i;
	mpz_t cpi;

	mpz_init(cpi);
	for (i=0; i<=64; i++) {
		mpz_init(facts[i]);
		mpz_fac_ui(facts[i], i);
	}

	while (fgets(fen, sizeof(fen) - 1, stdin) != NULL) {
		gmp_sscanf(fen, "%Zd\n", cpi);
//		gmp_printf("read CPI diagram: %Zd\n", cpi);
		i = write_diagram(&cpi, p.diagram);
		if (i==0) {
			puts("Error reading CPI (wow!)");
			continue;
		}
//		print_position(&p, 0);
		write_fen(&p, fen, 0);
		printf("%s\n", fen);
	}
	return 0;
}
