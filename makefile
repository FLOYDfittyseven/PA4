CC = gcc
CFLAGS = -Wall -Werror -g -pedantic -pthread -D__USE_POSIX -std=gnu99
COMPILE = $(CC) $(CFLAGS)

all: server5 client4

server5: server5.o bank.o
	$(COMPILE) -o server5 server5.c bank.c

client4: client4.o
	$(COMPILE) -o client4 client4.c

server5.o: server5.c
	$(COMPILE) -c server5.c
	
client4.o: client4.c
	$(COMPILE) -c client4.c

bank.o: bank.c bank.h
	$(COMPILE) -c bank.c

clean:
	rm -rf *.o server5 client4
