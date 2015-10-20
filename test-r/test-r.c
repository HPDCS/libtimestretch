/**
* this is a test case for verifyig correct behavior of the timestretch API in relation to 
* the attemp to execute 'anomalous' operations such as double open of the special device file and
* temination with no closure 
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <overtick.h>

#define DELAY 10000000

pthread_t tid[CPU_CORES];

void* addr = 0xf0f0f0f;
int ret;
int dummy;

//extern void overtick_callback(void);
extern void callback_entry(void);
extern void audit(void);


void *RegisterThread(void *arg){

	int i,j;
	char *p;
	unsigned me;
	char c[200];
	
	if(register_ts_thread() == TS_REGISTER_OK){ 
		printf("Thread %u registered\n",me=pthread_self());
		fflush(stdout);
		//	sleep(1);

		if(register_callback(callback_entry) == TS_REGISTER_CALLBACK_OK){
			printf("Thread %u registered callback\n",pthread_self());
		}
		else{
			printf("Thread %u register callback error\n",pthread_self());
		}


		for (j=0; j<200; j++){
		 for (i=0; i<DELAY; i++);

			printf("hello world %d times from %u\n",j,me);
			fflush(stdout);

			p = malloc(128);
			if (p) {
				sprintf(p,"hello world %d times\n",j);
				//printf("hello world %d times\n",i);
				//fflush(stdout);
			}

		}


		if(deregister_ts_thread() == TS_DEREGISTER_OK){ 
			printf("Thread %u deregistered\n",pthread_self());
		}
		else{
			printf("Thread %u deregister error\n",pthread_self());
		}
	}
	else{
		printf("Thread %u register error\n",pthread_self());
	}


	return (void*)&dummy;
}


int main(int argc, char **argv) {
	int i;
	int a;
	void * status;
	
//	printf("I'm process %d\n",getpid());


	if (argc<2){
		printf("missing  'arg'\n");
		printf("usage: prog arg\n");
		return -1;
	}

	if (!strcmp(argv[1],"doubleopen")){

		ret = ts_open();
		if(ret != TS_OPEN_ERROR) {
			printf("Device opened once.\n");
		} else {
			printf("Device open error.\n");
		}

		ret = ts_open();
		if((ret == TS_OPEN_OK)) {
			printf("Device opened twice (should never happen).\n");
		} else {
			printf("Device open error.\n");
		}

	}

	if (!strcmp(argv[1],"tryregister")){

		ret = ts_open();
		if(ret != TS_OPEN_ERROR) {
			printf("Device opened once.\n");
		} else {
			printf("Device open error.\n");
		}

		for (i=0;i<CPU_CORES;i++){
			a = pthread_create((tid+i), NULL, RegisterThread, NULL );
			if (a){
     			 printf("cannot create register thread for error %d\n", i);
      			 exit(-1);
	      		}
		}
	}

//	pause();
	sleep(12);
	//ts_close();	

	audit();
	

	return 0;
}

