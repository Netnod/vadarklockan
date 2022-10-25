SRCDIR := ../../src/c

CC := gcc
CFLAGS := -Wall -g -O2 -I$(SRCDIR)

all: vak_client

vak_client: vak_client.c $(SRCDIR)/overlap_algo.c $(SRCDIR)/vrt.c $(SRCDIR)/tweetnacl.c
	$(CC) $(CFLAGS) -o $@ $+

test: vak_client
	valgrind -s --leak-check=yes ./vak_client

clean:
	rm -f vak_client core *~ *.o


