CC=gcc
FLAGS=-Wall

all: sort_1 sort_2 sort_3 sort_4

sort_2: sort_2.c
	${CC} ${FLAGS} -o sort_2.o sort_2.c -I. -fopenmp

sort_4: sort_4.c
	${CC} ${FLAGS} -o sort_4.o sort_4.c -I. -fopenmp

sort_1: sort_1.c
	${CC} ${FLAGS} -o sort_1.o sort_1.c -I. -fopenmp

sort_3: sort_3.c
	${CC} ${FLAGS} -o sort_3.o sort_3.c -I. -fopenmp

clean:
	rm -f sort_1.o sort_2.o sort_3.o sort_4.o
