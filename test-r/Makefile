#try pass NCORES = the number of threads you wich to run in the example code
#the cpu consumer is compiled ignoring this value except if FLAGS is added to the compilatio line of this module

OBJ_DIR=../lib/

ifndef NCORES
	FLAGS = -D CPU_CORES=1
else
	FLAGS = -D CPU_CORES=$(NCORES)
endif


usage:
	echo 'make command [NCORES=number-of-cpucores]' 

example:
	gcc -c test-r.c $(FLAGS)  -g
	gcc -c callback.c $(FLAGS)  -g
	gcc -c entry.S $(FLAGS)  -g
	gcc -o test-r -g test-r.o callback.o entry.o $(OBJ_DIR)timestretchlib.o -lpthread

clean:
	rm test-r 

all:  example
