CC = gcc

CFLAGS = -Wall -g -ansi -pedantic

LD = gcc

LDFLAGS = 

mytar all: mytar.o functions.o bytestream.o specialint.o
	$(LD) $(LDFLAGS) -o mytar mytar.o functions.o bytestream.o specialint.o -lm

mytar.o: mytar.c
	$(CC) $(CFLAGS) -c -o mytar.o mytar.c

functions.o: functions.c
	$(CC) $(CFLAGS) -c -o functions.o functions.c

bytestream.o: bytestream.c
	$(CC) $(CFLAGS) -c -o bytestream.o bytestream.c

specialint.o: specialint.c
	$(CC) $(CFLAGS) -c -o specialint.o specialint.c

clean: 
	rm *.o mytar
