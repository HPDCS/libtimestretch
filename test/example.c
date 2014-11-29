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

#include <timestretch.h>

#define DELAY 500000000


int main(int argc, char **argv) {
	int i;
	int ret;
	
	printf("I'm process %d\n",getpid());

	ret = ts_open();
	if(ret != TS_OPEN_ERROR) {
		printf("Module is active.\n");
		register_ts_thread();
		ts_start(3000);
	} else {
		printf("Module is inactive.\n");
	}

	printf("Running my long lasting loop... ");
	fflush(stdout);
	for (i=0;i<DELAY;i++);
	printf("done\n");


	if(ret != TS_OPEN_ERROR) {
		ts_end();
		deregister_ts_thread();
	}

	return 0;
}
