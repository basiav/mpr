CC=gcc
FLAGS=-Wall

all: sort_2 sort_4

sort_2: sort_2.c
	${CC} ${FLAGS} -o sort_2.o sort_2.c -I. -fopenmp

sort_4: sort_4.c
	${CC} ${FLAGS} -o sort_4.o sort_4.c -I. -fopenmp