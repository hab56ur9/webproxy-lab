

CC = gcc
CFLAGS = -O2 -g -Wall -I .

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

all: tiny cgi proxy

tiny: tiny.c csapp.o
	$(CC) $(CFLAGS) -o tiny tiny.c csapp.o $(LIB)

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

cgi:
	(cd cgi-bin; make)

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o csapp.o
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

echoclient: echoclient.c csapp.o
	$(CC) $(CFLAGS) -o echoclient echoclient.c csapp.o $(LIB)

echoserver: echoserver.c csapp.o
	$(CC) $(CFLAGS) -o echoserver echoserver.c csapp.o $(LIB)


clean:
	rm -f *.o tiny *~
	(cd cgi-bin; make clean)
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz
	rm -f *.o echoclient echoserver *~

