/*
 * Converts FEN to CPI based on an ordering given in README
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
const int fact[11] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800};

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


int read_kings(mpz_t *cpi, const int diagram[64], int *wkp, int *bkp);
int read_armies(mpz_t *cpi, const int diagram[64], int all, int ek);
int read_permutation(mpz_t *cpi, const int diagram[64]);
int read_diagram(mpz_t *cpi, const int diagram[64]);


/* Adds to CPI the number of positions up to ones with kings placed as in POSITION
   King positions are saved to WKP, BKP
   Returns 0 if king placement is invalid 
*/
int read_kings(mpz_t *cpi, const int diagram[64], int *wkp, int *bkp)
{
	int i, j;
	mpz_t king_both_edge, king_one_edge, king_neither_edge;

#ifdef DEBUG
	//mpz_inits(king_both_edge, king_one_edge, king_neither_edge);
	mpz_init(king_both_edge);
	mpz_init(king_one_edge);
	mpz_init(king_neither_edge);
	read_armies(&king_both_edge, NULL, 1, 2);
	gmp_printf("*** set king_both_edge = %Zd\n", king_both_edge);
	read_armies(&king_one_edge, NULL, 1, 1);
	gmp_printf("*** set king_one_edge = %Zd\n", king_one_edge);
	read_armies(&king_neither_edge, NULL, 1, 0);
	gmp_printf("*** set king_neither_edge = %Zd\n", king_neither_edge);
#else
	mpz_init_set_str(king_both_edge, "7394384359052019860110129861127614655263725", 10);
	mpz_init_set_str(king_one_edge, "6499245207374987910384446769196893420982287", 10);
	mpz_init_set_str(king_neither_edge, "5710094948882734570085737442865185909590743", 10);
#endif

	/* Which square is each king on? */
	*wkp = *bkp = 99;
	for (i=0; i<64; i++) {
		if (diagram[i] == wk) 
			*wkp=i;
		if (diagram[i] == bk) 
			*bkp=i;
	}
	
	/* Traverse all possible king positions in README FILE order */
	for (i=0; i<64; i++) {
		for (j=0; j<64; j++) {
			if (abs(i%8-j%8)<=1 && abs(i/8-j/8)<=1) 
				continue; /* touching kings */
			if (i==*wkp && j==*bkp)
				return 1;
			if ((i/8)%7==0 && (j/8)%7==0)
				mpz_add(*cpi, *cpi, king_both_edge);
			else if ((i/8)%7==0 || (j/8)%7==0)
				mpz_add(*cpi, *cpi, king_one_edge);
			else
				mpz_add(*cpi, *cpi, king_neither_edge);
		}
	}
	
	return 0; /* kings in POSITON are not one of the valid ones traversed above */
}

/* Adds to CPI the number of positions up to ones with armies as in POSITION, where:
   -- King placement is given
   -- EK number of edge kings
   Returns 0 if armies are invalid 
   If ALL is non zero, ignore POSITIONS and count for all possible armies
*/
int read_armies(mpz_t *cpi, const int diagram[64], int all, int ek)
{
	mpz_t current;
	int targetarmies[NUMPIECETYPES] = {0};
	int armies[NUMPIECETYPES] = {0};
	int B, ABC, P; 	  /* Parameters for white */
	int b, abc, p;    /* Parameters for black */
	int allcap;
	int i;
	struct key *k;
	struct value *v, *found;
	struct hashtable *h;

	mpz_init(current);

	if (!all)
		for (i=0; i<64; i++)
			targetarmies[diagram[i]]++;

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

			if (!all)
				if (0 == memcmp(&targetarmies[wp], &armies[wp], sizeof(armies[wp])*(empty-wp))) {
					hashtable_destroy(h, 1);
					return 1;
				}

			p = fact[armies[bp]]*fact[armies[bq]]*fact[armies[br]]*fact[armies[bb]]*fact[armies[bn]];
		

			k->allspace = 62;
			k->pspace = 46+ek;
			k->numpawns = armies[wp] + armies[bp];
			k->numempty = 62 - ABC - abc;
			k->pieceperms = (int64_t)P*p;
			
			if ((found = search_some(h,k)))
			{
				mpz_add(*cpi, *cpi, *(((struct value *)found)->combs));
			}
			else
			{
				mpz_set(current, facts[k->pspace]);
				mpz_divexact(current, current, facts[k->pspace - k->numpawns]);
				mpz_mul(current, current, facts[k->allspace - k->numpawns]);
				mpz_divexact(current, current, facts[k->numempty]);
				mpz_divexact_ui(current, current, P);
				mpz_divexact_ui(current, current, p);
				mpz_add(*cpi, *cpi, current);

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

/* Adds to CPI the number of permutations of armies up to the one as in POSITION.
   Ignores pieces that are IGNORE
   Returns 0 if permutation is invalid
*/
int read_permutation(mpz_t *cpi, const int diagram[64])
{
	int i, j, k, allspace, pspace;
	int armies[NUMPIECETYPES];
	mpz_t current;

	mpz_init(current);

	memset(armies, 0, sizeof(armies[0])*NUMPIECETYPES);
	for (i=0; i<64; i++)
		armies[diagram[i]]++;

	for (i=0; i<64; i++) {
		if (diagram[i] == ignore)
			continue;
		for (j=wp; j<ignore; j++) {
			if ((j==wp || j==bp) && (i<8 || i>55))
				continue; /* a pawn can't go here */
			if (armies[j] <= 0)
				continue; /* no more of this piece left */

			armies[j]--;
			if (diagram[i] == j)
				break;		

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
			mpz_add(*cpi, *cpi, current);


			/* End Case */
			armies[j]++;
		}
//		gmp_printf(" -- CPI w/ diagram[%2d]: %Zd\n", i, *cpi);
	}

	i = 0;
	for (j=wp; j<ignore; j++)
		i += armies[j];
	return i==0; /* if not, that means we skipped a piece because it was placed invalidly */
}

/* 1 on success, 0 on invalid input */
int read_diagram(mpz_t *cpi, const int diagramorig[64])
{
	int wkp, bkp;  /* king positions */
	int ek; /* kings in edge position (1&8 rank) */
	int i;
	int diagram[64];

	mpz_set_ui(*cpi, 0);
	memcpy(diagram, diagramorig, sizeof(diagram[0])*64);

	i = read_kings(cpi, diagram, &wkp, &bkp);
	if (!i) return 0;
//	gmp_printf(" -- CPI with kings:     %Zd\n", *cpi);

	/* How many edge kings are there? */
	if ((wkp/8)%7==0 && (bkp/8)%7==0)
		ek=2;
	else if ((wkp/8)%7==0 || (bkp/8)%7==0)
		ek=1;
	else
		ek=0;

	i = read_armies(cpi, diagram, 0, ek);
	if (!i) return 0;
//	gmp_printf(" -- CPI with armies:    %Zd\n", *cpi);

	diagram[wkp] = ignore;
	diagram[bkp] = ignore;

	i = read_permutation(cpi, diagram);
	if (!i) return 0;
//	gmp_printf(" -- CPI with perms:     %Zd\n", *cpi);

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
		i = read_fen(fen, &p);
		if (i==0) {
			puts("Invalid FEN");
			continue;
		}
//		write_fen(&p, fen, 0);
//		print_position(&p, 0);
		i = read_diagram(&cpi, p.diagram);
		if (i==0) {
			puts("Invalid position");
			continue;
		}
		gmp_printf("%Zd\n", cpi);
	}
	return 0;
}
