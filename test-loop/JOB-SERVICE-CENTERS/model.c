#include <stdio.h>
#include "events.h"

struct _state state[NUM_CENTERS];

#define INITIAL_JOBS 10
#define INCREMENT 1.0
#define TRACE if(0)

#define PROCESSING_TIME 1.0

//#define BIAS 0.012
//#define BIAS 0.001
#define BIAS 0.0

int i;

#define AUDIT if(0)

#define DELAY_COUNT  20000000
#define DELAY for(i=0;i<DELAY_COUNT;i++); 

void audit_all(void){

	int i,j;
	system("clear");
	for (i=0;i<NUM_CENTERS;i++){
		printf("Center %d - ",i);
		for(j=0;j<state[i].queued_jobs;j++) printf("|");
		printf("\n");
	}
}


int ProcessEvent(msg_type msg){

	int i;
	msg_type new_msg;
	time_type now;
	int target_center;
	double value;

	now = msg.timestamp;
	target_center = msg.receiver;
	state[target_center].current_simulation_time = now;



	switch(msg.type){
	
		case INIT:
			TRACE
			printf("INIT on center %d\n",target_center);
			state[target_center].current_simulation_time =  0.0;
			state[target_center].queued_jobs =  0;
			state[target_center].served_jobs =  0;
			state[target_center].busy =  0;
			state[target_center].total_service_time =  0.0;

			for(i=0;i<INITIAL_JOBS;i++){
				new_msg.receiver = target_center;
				new_msg.sender = target_center;
				new_msg.type = JOB_ARRIVAL;
				new_msg.send_time = msg.timestamp;
				new_msg.timestamp = now + (i+1)*0.01*INCREMENT; 
				Schedule(new_msg);
			}
		break;


		case JOB_ARRIVAL:

			DELAY;

			TRACE
			printf("ARRIVAL ON center %d - time %e - served jobs %d - queued jobs %d server busy\n",
						target_center,
						state[target_center].current_simulation_time,
						state[target_center].served_jobs,
						state[target_center].queued_jobs,
						state[target_center].busy
				);


			state[target_center].queued_jobs++;
			if(state[target_center].busy == 0) {
				state[target_center].busy = 1;
				new_msg.receiver = target_center;
				new_msg.sender = target_center;
				new_msg.type = JOB_DEPARTURE;
				new_msg.send_time = now;
				new_msg.timestamp = now + BIAS * target_center + INCREMENT; 
				Schedule(new_msg);
				
			};	

			TRACE
			printf("PROCESSED ARRIVAL ON center %d - time %e - served jobs %d - queued jobs %d server busy\n",
						target_center,
						state[target_center].current_simulation_time,
						state[target_center].served_jobs,
						state[target_center].queued_jobs,
						state[target_center].busy
				);

		break;


		case JOB_DEPARTURE:

			DELAY;

			if((++state[target_center].served_jobs) % 100 == 0 ){
				TRACE
				printf("center %d - time %e - served jobs %d - queued jobs %d\n",
						target_center,
						state[target_center].current_simulation_time,
						state[target_center].served_jobs,
						state[target_center].queued_jobs
				);
				AUDIT
				audit_all();
			};

			state[target_center].queued_jobs--;
			state[target_center].busy = 0 ;
			if((state[target_center].queued_jobs)>0) {
				state[target_center].busy = 1 ;
				new_msg.receiver = target_center;
				new_msg.sender = target_center;
				new_msg.type = JOB_DEPARTURE;
				new_msg.send_time = now;
				new_msg.timestamp = now + BIAS * target_center + INCREMENT; 
				Schedule(new_msg);
			}

			TRACE
			printf("DEPARTURE FROM center %d - time %e - served jobs %d - queued jobs %d server busy\n",
						target_center,
						state[target_center].current_simulation_time,
						state[target_center].served_jobs,
						state[target_center].queued_jobs,
						state[target_center].busy
				);

			target_center++;
			target_center = target_center%NUM_CENTERS;

			TRACE
			printf("new target center is %d\n",target_center);

			new_msg.receiver = target_center;
			new_msg.sender = target_center;
			new_msg.type = JOB_ARRIVAL;
			new_msg.send_time = now;
			new_msg.timestamp = now + 0.01*INCREMENT; 
			Schedule(new_msg);

		break;



	}



}
