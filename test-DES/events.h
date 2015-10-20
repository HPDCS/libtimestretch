#ifndef EVENTS_DEFINITION
#define EVENTS_DEFINITION

typedef double time_type;
typedef unsigned long seed_type;

#define NUM_CENTERS	10 //user defined - based on model logical partitioning
#define MAX_EVENTS	100000 // user defined 	

#define INIT 		0 //mandatory 
#define JOB_ARRIVAL 	1
#define JOB_DEPARTURE 	2


typedef struct _msg_type{
  int   sender;
  int   receiver;
  int   type;
  time_type   send_time;
  time_type   timestamp; ///mandatory field
} msg_type;


typedef struct _state {

  time_type current_simulation_time;
  int       queued_jobs;
  int       served_jobs;
  int       busy;
  time_type total_service_time;

} state_type;

/*
typedef struct __PCS_routing{
  int num_adjacent;
  int adjacent_identities[6];
} _PCS_routing;

*/ 
#endif
