CC=gcc
FLAGS=-Wall -g

sort_2: sort_2.c
	${CC} ${FLAGS} -o sort_2.o sort_2.c -I. -fopenmp