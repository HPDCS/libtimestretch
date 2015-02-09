#try pass NCORES = the number of threads you wich to run in the example code
#the cpu consumer is compiled ignoring this value except if FLAGS is added to the compilatio line of this module

OBJ_DIR=../
ifndef NCORES
	FLAGS = -D CPU_CORES=1
else
	FLAGS = -D CPU_CORES=$(NCORES)
endif


usage:
	echo 'make command [NCORES=number-of-cpucores]' 

example:
	gcc -c example.c $(FLAGS)
	gcc -o example example.o $(OBJ_DIR)timestretchlib.o -lpthread

cpu-consumer:
	gcc -o cpu-consumer cpu-consumer.c  -lpthread

clean:
	rm example
	rm cpu-consumer

all: cpu-consumer example
