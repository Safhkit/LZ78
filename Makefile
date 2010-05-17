CC = gcc
CFLAGS = -Wall
OBJ = utility.o bit_io.o lz78.o main.o stack.o

all: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o lz78

bit_io.o: bit_io.c bit_io.h utility.c utility.h
	$(CC) $(CFLAGS) bit_io.c -c

utility.o: utility.h
	$(CC) $(CFLAGS) utility.c -c

stack.o: stack.c stack.h utility.c utility.h
	$(CC) $(CFLAGS) stack.c -c

lz78.o: utility.c utility.h bit_io.c bit_io.h lz78.c lz78.h stack.c stack.h
	$(CC) $(CFLAGS) lz78.c -c

main.o: utility.c utility.h bit_io.c bit_io.h lz78.c lz78.h main.c stack.c stack.h
	$(CC) $(CFLAGS) main.c -c

clean:
	rm -f *.o lz78