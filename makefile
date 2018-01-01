# Lauren Sherman
# Makefile

main: main.o
	gcc -o main main.o -lpthread
main.o: main.c
	gcc -ansi -pedantic -Wall -g -c main.c -lpthread 

clean:
	\rm -f *.o main
