CFLAGS+= -Wall -O3 -g
INCLUDE=-I/opt/local/include 
LINK=-L/opt/local/lib

all: ub

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $< $(INCLUDE)

%: %.c
	gcc $(CFLAGS) -o $@ $< $(INCLUDE) $(LINK) -lgmp

convert:	convert.o fen.o hashtable.o
	gcc $(CFLAGS) -o $@ $^ $(INCLUDE) $(LINK) -lgmp

convertback:	convertback.o fen.o hashtable.o
	gcc $(CFLAGS) -o $@ $^ $(INCLUDE) $(LINK) -lgmp

rand:	rand.o fen.o 
	gcc $(CFLAGS) -o $@ $^ $(INCLUDE) $(LINK) -lgmp

hashtable.o:	hashtable.c
	gcc $(CFLAGS) -c hashtable.c -o hashtable.o

hashtable_itr.o: hashtable_itr.c
	gcc $(CFLAGS) -c hashtable_itr.c -o hashtable_itr.o

ub.o:		ub.c
	gcc $(CFLAGS) -c -o $@ $< -I/opt/local/include

ub:		ub.o hashtable.o hashtable_itr.o
	gcc $(CFLAGS) -o ub ub.o hashtable.o hashtable_itr.o -L/opt/local/lib -lgmp

readfen:	readfen.o hashtable.o hashtable_itr.o
	gcc $(CFLAGS) -o readfen readfen.o hashtable.o hashtable_itr.o -L/opt/local/lib -lgmp

clean:
	rm -f *.o

#     foo : foo.c -lcurses
#:             cc $^ -o $@
