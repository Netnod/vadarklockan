SRCDIR := ../../src/c

CC := gcc
CFLAGS := -Wall -g -O2 -I$(SRCDIR)

# all: vak_client

vak_client_single: vak_client.c $(SRCDIR)/vak_main.c $(SRCDIR)/vak_impl_single.c $(SRCDIR)/vak_udp_linux.c $(SRCDIR)/vak_time_linux.c $(SRCDIR)/vak_random_linux.c $(SRCDIR)/vak_servers.c $(SRCDIR)/overlap_algo.c $(SRCDIR)/vrt.c $(SRCDIR)/tweetnacl.c
	$(CC) $(CFLAGS) -o $@ $+

test: vak_client_single
	valgrind -s --leak-check=yes ./vak_client_single

clean:
	rm -f vak_client_single core *~ *.o


