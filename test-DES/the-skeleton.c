#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>

#include <timestretch.h>


#include "eventlist.h"
#include "def.h"



#include "eventlist7-static.c"

//extern void overtick_callback(void);
extern void callback_entry(void);
extern void audit(void);


double last_time;




void RegisterThread( void * arg){

        int i;

        if(register_ts_thread() == TS_REGISTER_OK){
                printf("Thread %u registered\n",pthread_self());

                if(register_callback(callback_entry) == TS_REGISTER_CALLBACK_OK){
                //if(register_callback(NULL) == TS_REGISTER_CALLBACK_OK){
                        printf("Thread %u registered callback\n",pthread_self());
                }
                else{
                        printf("Thread %u register callback error\n",pthread_self());
                }
	}
	else{
                printf("Thread %u register error\n",pthread_self());
	}
}

void DeregisterThread(){

                if(deregister_ts_thread() == TS_DEREGISTER_OK){
                        printf("Thread %u deregistered\n",pthread_self());
                }
                else{
                        printf("Thread %u deregister error\n",pthread_self());
                }
}


#define BUF_SIZE	2048
char source_buff[BUF_SIZE];
char dest_buff[BUF_SIZE];

void *concurrent_thread(void *arg) {

	double a=1, b=2, c=3;

	RegisterThread(NULL);

	while(1) {
		c = pow(a, b);
		memcpy(dest_buff, source_buff, BUF_SIZE);
		c = log(c);
	}

	DeregisterThread();
}

void Schedule(msg_type msg){
	insert(msg);	
}

int main(int argc, char**argv){
 
   int i;
   msg_type event;
   msg_type current_event;
   int num_events;
   int ret;
   pthread_t tid;

   if (argc<2) {
	printf("usage: prog event-counter\n");
	return -1;
   }



   ret = ts_open();
   if(ret != TS_OPEN_ERROR) {
          printf("Device opened once.\n");
   } else {
          printf("Device open error.\n");
   }

   pthread_create(&tid, NULL, concurrent_thread, NULL);


   initialize_event_list();
   RegisterThread(NULL);
	
   for(i=0;i<NUM_CENTERS;i++){
	event.send_time = 0.0;
	event.timestamp = 0.0 + 0.01*i;
	event.type = INIT;
	event.sender = i;
	event.receiver = i;
   	insert(event);
   }

   num_events = atoi(argv[1]);

//RegisterThread(NULL);
   for(i=0;i<num_events;i++){
   	next(&current_event);
//	printf("schedule %d - event timestamp = %e\n",i,current_event.timestamp);
	last_time = current_event.timestamp;
//	RegisterThread(NULL);
	ProcessEvent(current_event);
//	DeregisterThread();
   }

   DeregisterThread();
   printf("done all work - last time is %e\n",last_time); 
   audit();
 
}
