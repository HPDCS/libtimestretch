#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <malloc.h>


#include "eventlist.h"
#include "def.h"



#include "eventlist7-static.c"
//#include "eventlist6.c"


msg_type event;
msg_type current_event;

void Schedule(msg_type msg){
	insert(msg);	
}

int main(){
 
   int i;

   initialize_event_list();

   for(i=0;i<10000;i++){
	event.send_time = 0.1*i;
	event.timestamp = 0.2*i;
   	insert(event);
   	next(&current_event);
	printf("schedule %d - event timestamp = %e\n",i,current_event.timestamp);
   }

   return;
   for(i=0;i<1000;i++){
   	next(&current_event);
	printf("schedule %d - event timestamp = %e\n",i,current_event.timestamp);
   }
   printf("done all work\n");
}
