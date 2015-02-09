#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef CPU_CORES
#define CPU_CORES 1
#endif

pthread_t tid[CPU_CORES];

int dummy;

void * BusyLoop(void*arg){
	int i;

	while(1){
		for(i=0;i<1000000000;i++);
//		printf("I'm alive\n");
	}
	return (void*)&dummy;
}

int main(void) {

	int i;
	int a;
	void * status;

	if(CPU_CORES == 1){
	 	BusyLoop(NULL);
		return 0;
	}

	for (i=0;i<CPU_CORES;i++){
                a = pthread_create((tid+i), NULL, BusyLoop, NULL );
                if (a){
                 printf("cannot create thread for error %d\n", i);
                 exit(-1);
                }
        }
        for (i=0;i<CPU_CORES;i++){
                pthread_join(tid[i], &status);  
        }

}
