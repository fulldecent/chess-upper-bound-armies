#include<stdio.h>
#include<stdlib.h>

extern double pow(double, double);

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

typedef struct
{
    int n;        /* number of pieces */
    int np;
    int nq;
    int nr;
    int nb;
    int nn;
} PIECES;    /* number of pieces of each type */

#define MAX_COMBOS 30000

#define MAX_PAWNS 8
#define MAX_QUEENS 9
#define MAX_ROOKS 10
#define MAX_BISHOPS 10
#define MAX_KNIGHTS 10

double fs[65];
double cs[65][65];
double pawns[9][9];

PIECES W, B;
double curmax = 0.0;

fact()
{
    int i, j;

    /* computes and stores factorials from 0 to 64 */

    for (i=0; i<65; i++)
    {
        fs[i] = 1.0;
        for (j=2; j<=i; j++)
        {
            fs[i] *= j;
        }
    }
}


choose()
{
    int i, j;

    /* computes C(i,j) for 0 <= i,j < 65 */

    for (i=0; i<65; i++)
    {
        for (j=0; j<65; j++)
        {
            if (i >= j)
            {
                cs[i][j] = fs[i] / fs[j] / fs[i-j];      /* i choose j */
            }
            else
            {
                cs[i][j] = -1.0;
            }
        }
    }
}

place_pawns()
{
    /* places W and B pawns.  Takes into account enpassant and who to move */

    int w, b, i, j, k, l;
    double m;

    for (w=0; w<=8; w++)
    {
        for (b=0; b<=8; b++)
        {
            pawns[w][b] = 0.0;
            for (i=0; i<=w; i++)
            {
                for (j=0; j<=(w-i); j++)
                {
                    for (k=0; k<=b && k<=(8-j); k++)
                    {
                        for (l=0; l<=(b-k) && l<=(8-i); l++)
                        {
                            m = cs[8][i] * cs[8][j] * cs[32][w-i-j];
                            m *= cs[8-j][k] * cs[8-i][l];
                            m *= cs[32-(w-i-j)][b-k-l];
                            m *= (2.0 + min(2*l,i) + min(2*j,k));
                            pawns[w][b] += m;
                        }
                    }
                }
            }
        }
    }
}

double place_rooks(int l, int r)
{
    /* places K and R */

    double c1, c2;

    c1 = l * cs[l-1][r];         /* king not home. no castling */
                               /* Not (l-1) because K's home may be occupied
                                  by other King (B case only) */

    c2 = cs[l-1][r];                   /* king home. */
    if (r == 1) c2 *= 3.0;             /* castling */
    if (r >= 2) c2 *= 4.0;

    return c1 + c2;
}

double arrange(int Wn, int Wnp, int Wnq, int Wnr, int Wnb, int Wnn,
               int Bn, int Bnp, int Bnq, int Bnr, int Bnb, int Bnn,
               int Wpot, int Bpot)
{
    double M;
    int left;
    int nblocked;

    left = 64;
    M = 1.0;

    if (Wnp - Wpot != Bnp - Bpot) printf("error\n"), exit(1);

    nblocked = Wnp - Wpot;   /* number of files guaranteed to be blocked */
    if (nblocked != 0)
    {
        /* choose blocked files: within each file, there are 15 ways of
           placing 1 white and 1 black pawn */
        M *= cs[8][nblocked] * pow((double) 15.0, (double) nblocked);
        /* place remaining white pawns */
        M *= cs[48-2*nblocked][Wnp - nblocked];
        /* place remaining black pawns */
        M *= cs[48-Wnp-nblocked][Bnp - nblocked];
        /* enpassant upper bound + who to move */
        M *= (1.0 + min(2*Wnp,Bnp) + 1.0 + min(2*Bnp,Wnp));
    }
    else
    {
        M *= pawns[Bnp][Wnp];     /* pawns, enpassant, who to move */
    }
    left -= Wnp + Bnp;

    M *= place_rooks(left, Wnr);       /* W rook and K */
    left -= Wnr + 1;
    M *= place_rooks(left, Bnr);       /* B rook and K */
    left -= Bnr + 1;

    M *= cs[left][Wnb];       /* W bishops */
    left -= Wnb;
    M *= cs[left][Bnb];       /* B bishops */
    left -= Bnb;

    M *= cs[left][Wnq];       /* W queen */
    left -= Wnq;
    M *= cs[left][Bnq];       /* B queen */
    left -= Bnq;

    M *= cs[left][Wnn];       /* W knights */
    left -= Wnn;
    M *= cs[left][Bnn];       /* W knights */
    left -= Bnn;

    if (left != 64 - Wn - Bn) printf("%d %d %d\n", left, Wn, Bn), exit(1);

    if (M > curmax)
    {
        curmax = M;
        W.n = Wn;
        W.np = Wnp;
        W.nq = Wnq;
        W.nr = Wnr;
        W.nb = Wnb;
        W.nn = Wnn;
        B.n = Bn;
        B.np = Bnp;
        B.nq = Bnq;
        B.nr = Bnr;
        B.nb = Bnb;
        B.nn = Bnn;
    }
    return M;
}


main()
{
    int ncombos;  /* # different combinations of pieces of a single color */

    PIECES pieces[MAX_COMBOS];
    int np, nq, nr, nb, nn;
    int i, j;
    double N;
    int npromos, nWpromos, nBpromos, ncaptures;
    int nWcaptures, nBcaptures;
    int nWpieces_captured, nWpawns_captured;
    int nBpieces_captured, nBpawns_captured;
    int Wpot, Bpot;        /* promotion potential (ie # non-blocked pawns) */

    int nsets = 0;

    ncombos = 0;

    for (np=0; np<=MAX_PAWNS; np++)
    {
        for (nq=0; nq<=MAX_QUEENS; nq++)
        {
            for (nr=0; nr<=MAX_ROOKS; nr++)
            {
                for (nb=0; nb<=MAX_BISHOPS; nb++)
                {
                    for (nn=0; nn<=MAX_KNIGHTS; nn++)
                    {
                        if (max(nq-1,0) + max(nr-2,0) + max(nb-2,0)
                           + max(nn-2,0) <= 8 - np)
                        {
                            pieces[ncombos].n = np + nq + nr + nb
                                              + nn + 1;  /* K inc'd */
                            pieces[ncombos].np = np;
                            pieces[ncombos].nq = nq;
                            pieces[ncombos].nr = nr;
                            pieces[ncombos].nb = nb;
                            pieces[ncombos].nn = nn;
                            ncombos++;
                            if (ncombos >= MAX_COMBOS)
                            {
                                printf("Out of space\n");
                                exit(1);
                            }
                        }
                    }
                }
            }
        }
    }
    printf("Number of combinations of 1 color = %d\n\n", ncombos);

    /* now for each of these combos, find how many arrangements exist */

    N = 0.0;
    fact();
    choose();
    place_pawns();

    for (i=0; i<ncombos; i++)
    {
        /* white pieces */

        nWpromos = max(pieces[i].nq-1,0) + max(pieces[i].nr-2,0)
                 + max(pieces[i].nb-2,0) + max(pieces[i].nn-2,0);
        nWcaptures = 16 - pieces[i].n;
        nWpawns_captured = 8 - pieces[i].np - nWpromos;
        nWpieces_captured = nWcaptures - nWpawns_captured;

        if (nWpromos < 0 || nWcaptures < 0 || nWpawns_captured < 0 ||
            nWpieces_captured < 0) printf("Error\n"), exit(1);

        for (j=0; j<ncombos; j++)
        {
            /* black pieces */

            nBpromos = max(pieces[j].nq-1,0) + max(pieces[j].nr-2,0)
                     + max(pieces[j].nb-2,0) + max(pieces[j].nn-2,0);

            npromos = nWpromos + nBpromos;
            nBcaptures = 16 - pieces[j].n;
            nBpawns_captured = 8 - pieces[j].np - nBpromos;
            nBpieces_captured = nBcaptures - nBpawns_captured;

            if (nBpromos < 0 || nBcaptures < 0 || nBpawns_captured < 0 ||
                nBpieces_captured < 0) printf("Error\n"), exit(1);

            ncaptures = nWcaptures + nBcaptures;

            if (nWcaptures + nBpieces_captured + 2*nBpawns_captured
                                   >= nWpromos &&
                nBcaptures + nWpieces_captured + 2*nWpawns_captured
                                   >= nBpromos)
            {
                Wpot = nWcaptures + nBpieces_captured + 2*nBpawns_captured
                     - nWpromos;
                Wpot = min(Wpot, pieces[i].np);
                Bpot = nBcaptures + nWpieces_captured + 2*nWpawns_captured
                     - nBpromos;
                Bpot = min(Bpot, pieces[j].np);

                N += arrange(pieces[i].n, pieces[i].np, pieces[i].nq,
                             pieces[i].nr, pieces[i].nb, pieces[i].nn,
                             pieces[j].n, pieces[j].np, pieces[j].nq,
                             pieces[j].nr, pieces[j].nb, pieces[j].nn,
                             Wpot, Bpot);
                nsets++;
            }
        }
        if (i == ncombos-1 || i % 1000 == 0)
        {
            printf("number examined = %d : total positions = %E\n", i+1, N);

            printf("current maximum = %E\n", curmax);
            printf("for combination of pieces given below:\n");
            printf("W: P = %d Q = %d R = %d B = %d N = %d\n",
                   W.np, W.nq, W.nr, W.nb, W.nn);
            printf("B: P = %d Q = %d R = %d B = %d N = %d\n",
                   B.np, B.nq, B.nr, B.nb, B.nn);
            printf("\n");
        }
    }

    printf("%d combinations examined\n", nsets);
}
