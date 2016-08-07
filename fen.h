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

const char * PIECES;

struct position;
int read_fen(const char *fen, struct position *p);
void write_fen(const struct position *p, char *fen, int full);
void print_position(struct position * p, int full);
