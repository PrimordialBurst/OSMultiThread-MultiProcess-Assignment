CC = gcc
CFLAGS = -g -lrt -lm -pthread
OBJ = main.o

mssv : $(OBJ)
	$(CC) -pthread -lrt -o mssv $(OBJ)

main.o: main.c
	$(CC) $(CFLAGS) -pthread -lrt -c main.c -lrt
clean :
	rm -f mssv $(OBJ)
