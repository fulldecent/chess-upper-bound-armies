/*
 * Read and write chess positions from FEN format
 *
 * This program will not definitely reject invalid FENs.
 *
 * François Labelle provides exact definitions of diagrams and positions at 
 * http://www.eecs.berkeley.edu/~flab/chess/statistics-positions.html
 */

#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

const char * PIECES = "KkPQRBNpqrbn.";

struct position {
    int diagram[64];   /* rank 8 to 1, file A to H */
    int color;         /* w=0, b=1 */
    int castling[4];   /* KQkq, no=0, yes=1 */
    char enpassant;    /* '-' for none, 'a'..'h' */
};

/* Return value:
     0 -- Error in input
     1 -- Only a diagram was read
     2 -- A full position was read
*/
int read_fen(const char *fen, struct position *p)
{
    int i = 0;
    int fenindex = 0;
    int diaindex = 0;
    p->color = 0;
    p->castling[0] = 0;
    p->castling[1] = 0;
    p->castling[2] = 0;
    p->castling[3] = 0;
    p->enpassant = 0;

    /* Load the board */
    while (!isspace(fen[fenindex]) && fenindex < 70) {
        if (diaindex >= 64) return 0;
        if (fen[fenindex] == '/') {
            fenindex++;
            continue;
        }
        if (fen[fenindex] >= '1' && fen[fenindex] <= '8') {
            i = fen[fenindex];
            while (i-- > '0')
                p->diagram[diaindex++] = 12; /* WARNING TODO THIS IS A HARD CODED SPACE */
            fenindex++;
            continue;
        }
        for (i=0; i<12 && PIECES[i] != fen[fenindex]; i++);
        if (i >= 12) return 0;
        p->diagram[diaindex++] = i;
        fenindex++;
    }
    if (diaindex != 64) return 0;
    if (fen[fenindex] == '\0' || fen[fenindex] == '\x0a' || fen[fenindex] == '\x0d')
        return 1;
    fenindex++;                   /* Skip a space */

    /* Load the color to move */
    if (fen[fenindex] == 'w')
        p->color = 0;
    else if (fen[fenindex] == 'b')
        p->color = 1;
    else
        return 0;
    fenindex += 2;                /* ALSO Skip a space */

    /* Load castling */
    memset(&p->castling, 0, sizeof(p->castling));
    for (i = 0; i < 4; i++) {
        if (fen[fenindex] == 'K')
            p->castling[0] = 1;
        else if (fen[fenindex] == 'k')
            p->castling[1] = 1;
        else if (fen[fenindex] == 'Q')
            p->castling[2] = 1;
        else if (fen[fenindex] == 'q')
            p->castling[3] = 1;
        else if (fen[fenindex] == '-');
        else
            break;
        fenindex++;
    }
    fenindex++;                   /* Skip a space */

    /* Load en passant */
    if (fen[fenindex] == '-')
        p->enpassant = '-';
    else if (fen[fenindex] >= 'a' && fen[fenindex] <= 'h')
        p->enpassant = fen[fenindex++];
    else
        return 0;
    return 2;
}

/* full==0: print only diagram, full==1: print full position */
void write_fen(const struct position *p, char *fen, int full)
{
    int i = 0, j = 0;
    int fenindex = 0;

    for (i = 0; i < 64; i++) {
        if (i % 8 == 0 && i > 0) {
            if (j) {
                fen[fenindex++] = '0' + j;
                j = 0;
            }
            fen[fenindex++] = '/';
        }
        if (p->diagram[i] == 12) /* WARNING TODO HARDCODED SPACE */
            j++;
        else {
            if (j) {
                fen[fenindex++] = '0' + j;
                j = 0;
            }
            fen[fenindex++] = PIECES[p->diagram[i]];
        }
    }
    if (j) {
        fen[fenindex++] = '0' + j;
    }

    if (!full) {
        fen[fenindex++] = '\0';
        return;
    }
    fen[fenindex++] = ' ';

    /* Save the color to move */
    fen[fenindex++] = p->color ? 'b' : 'w';
    fen[fenindex++] = ' ';

    /* Save castling */
    if (p->castling[0])
        fen[fenindex++] = 'K';
    if (p->castling[1])
        fen[fenindex++] = 'Q';
    if (p->castling[2])
        fen[fenindex++] = 'k';
    if (p->castling[3])
        fen[fenindex++] = 'q';
    if (!p->castling[0] && !p->castling[1] && !p->castling[2]
        && !p->castling[3])
        fen[fenindex++] = '-';
    fen[fenindex++] = ' ';

    /* Save en passant */
    fen[fenindex++] = p->enpassant;
    if (p->enpassant != '-')
      fen[fenindex++] = p->color ? '3' : '6';
    fen[fenindex++] = ' ';

    fen[fenindex++] = '0'; /* halfmove */
    fen[fenindex++] = ' ';
    fen[fenindex++] = '0'; /* fullmove */
}

/* full==0: print only diagram, full==1: print full position */
void print_position(struct position * p, int full)
{
    int i;
    puts("DIAGRAM:");
    for (i = 0; i < 64; i++) {
        putc(PIECES[p->diagram[i]], stdout);
        if (i % 8 == 7)
            puts("");
    }
    if (!full) return;
    printf("\nCOLOR:      %c\n", p->color ? 'b' : 'w');
    printf("CASTLING:   ");
    if (p->castling[0]) putc('K', stdout);
    if (p->castling[1]) putc('Q', stdout);
    if (p->castling[2]) putc('k', stdout);
    if (p->castling[3]) putc('q', stdout);
    if (!p->castling[0] && !p->castling[1] && !p->castling[2]
        && !p->castling[3])
        putc('-', stdout);
    printf("\nEN PASSANT: %c\n", p->enpassant);
}

/*
int main()
{
    char fen[1024] = { 0 };
    struct position p;
    int i;

    while (fgets(fen, sizeof(fen) - 1, stdin) != NULL) {
        i = read_fen(fen, &p);
        print_position(&p, i);
        write_fen(&p, fen, i);
        puts(fen);
    }

    puts("Exiting.");
    return 0;
}
*/
