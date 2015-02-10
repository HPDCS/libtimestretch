/**
 * This is a simple example to show the correct usage of libtimestretch.
 * This software simply executes a long loop, which is expected to require
 * more that a single time quantum.
 * Using libtimestretch can make the execution time of this software about
 * one half.
 * To check that, simply run this same software with or without the module
 * installed. Additionally, use taskset to specify affinity to a core
 * different than zero. Measuring execution time with `time` should give
 * the desired result, when some "noise" (as the one produced by cpu-consumer)
 * is in place, as in the following example:
 *
 *     user@debian:~/timer-stretch-lib/trunk/test$ ./cpu-consumer & ./cpu-consumer &
 *     user@debian:~/timer-stretch-lib/trunk/test$ time ./example
 *     I'm process 3536
 *     Module is inactive.
 *     Running my long lasting loop... done
 *     
 *     real    0m8.323s
 *     user    0m5.228s
 *     sys     0m0.000s
 *     user@debian:~/timer-stretch-lib/trunk/test$ sudo modprobe timestretch
 *     user@debian:~/timer-stretch-lib/trunk/test$ time taskset 0x2 ./example
 *     I'm process 3615
 *     Module is active.
 *     Running my long lasting loop... done
 *     
 *     real    0m5.643s
 *     user    0m4.976s
 *     sys     0m0.000s
 *     user@debian:~/timer-stretch-lib/trunk/test$ sudo rmmod timestretch
 *     user@debian:~/timer-stretch-lib/trunk/test$ killall cpu-consumer
 *     [2]+  Terminated              ./cpu-consumer
 *     [1]+  Terminated              ./cpu-consumer
 *     user@debian:~/timer-stretch-lib/trunk/test$ time taskset 0x2 ./example
 *     I'm process 3634
 *     Module is inactive.
 *     Running my long lasting loop... done
 *     
 *     real    0m5.418s
 *     user    0m5.392s
 *     sys     0m0.012s
 *     
 * By this example it is clear that using libtimestretch you can have with contention
 * the same execution time as if there were no contention at all, upon request.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <timestretch.h>

#include "timer.h"

#define DELAY 120000000

pthread_t tid[CPU_CORES];

int ret;
int dummy;

void * BusyLoopThread( void * arg){

	int i;
	int j;

	DECLARE_TIMER(mary);	

	printf("Thread %d - Running my long lasting loop... ", pthread_self());
        //fflush(stdout);
	
	register_ts_thread();

for(j=0;j<10;j++){
	ts_start(1000);
	TIMER_START(mary);
        for (i=0;i<DELAY;i++);
	TIMER_VALUE(diff,mary);
	ts_end();
	printf("Thread %d - done\n",pthread_self());
	TIMER_PRINT(diff);
}


	deregister_ts_thread();

	return (void*)&dummy;

}

void * BusyLoopThreadNoStretch( void * arg){

	int i;
	int j;

	DECLARE_TIMER(mary);	

	printf("Thread %d - Running my long lasting loop... ", pthread_self());
        //fflush(stdout);
	

for(j=0;j<10;j++){
	TIMER_START(mary);
        for (i=0;i<DELAY;i++);
	TIMER_VALUE(diff,mary);
	printf("Thread %d - done\n",pthread_self());
	TIMER_PRINT(diff);
}

}

int main(int argc, char **argv) {
	int i;
	int a;
	void * status;
	
	printf("I'm process %d\n",getpid());
	ret = ts_open();
	if(ret != TS_OPEN_ERROR) {
		printf("Module is active.\n");
	} else {
		printf("Module is inactive.\n");
	}

	if(CPU_CORES == 1){

		BusyLoopThreadNoStretch(NULL);
		BusyLoopThread(NULL);
		return 0;
	}

	for (i=0;i<CPU_CORES;i++){
		a = pthread_create((tid+i), NULL, BusyLoopThread, NULL );
		if (a){
     		 printf("cannot create thread for error %d\n", i);
      		 exit(-1);
      		}
	}
	for (i=0;i<CPU_CORES;i++){
		pthread_join(tid[i], &status);	
	}

	return 0;
}

