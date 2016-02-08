APP = ./JOB-SERVICE-CENTERS

OBJ_DIR=../lib/

all:
	cd $(APP);gcc -c model.c -I ..
	gcc -c the-skeleton.c
	gcc -c callback.c $(FLAGS) 
	gcc -c entry.S $(FLAGS)  
	gcc -o simulator the-skeleton.o entry.o callback.o $(APP)/model.o -lpthread -lm -ltimestretch
