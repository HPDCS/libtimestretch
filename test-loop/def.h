#define  NORM                  0x7fffffff



/************************* INIT TIME ***************************/


#define  INIT_SIMULATION_TIME	   0.0
#define  END_SIMULATION_TIME	 1000.0



/**************************************************/

#define  OUT_VALUE             -1


#define  FALSE                 0
#define  TRUE                  1

#define  FREE			-101
#define  OCCUPIED		-102


#define  EV_LIST_LENGTH		        40000
#define  MAX_GLOBAL_QUEUE_LENGTH	10000



/******************** NETWORK DIMENSION ******************************/

#define DIMENSION 				100
#define NODE_QUEUE_LENGTH			1000
#define FLIT_TRANSMISSION_TIME			1.0
#define NUM_DIRECTIONS				4
#define NUM_VIRTUAL_CHANNELS_PER_DIRECTION	4  /* CHANNELS  2  AND  3  ARE DESTINED TO WRAPAROUND MESSAGES */

/* #define INTERARRIVAL_TIME			1000   */


/******************* DEFINITION OF EVENTS ****************************/


#define  NEW_MESSAGE	      	       102

#define  END_FLIT_TRANSMISSION  	99

#define  FLIT_ARRIVAL   		95

#define	 CHECK_TRANSMIT			93

#define  FREE_PHYSICAL_CHANNEL		92


/****************** DEFINITION OF FLIT TYPES ************************/

#define  HEADER_FLIT	-99
#define	 TAIL_FLIT	-98
#define	 DATA_FLIT	-97

/***************** DEFINITION OF FLIT STATUS ***********************/

#define  TO_BE_TRANSMITTED   -96
#define  TRANSMITTED	   -95

/***************** DEFINITION OF DIRECTIONS ***********************/

#define  NORD	   0
#define  SUD	   1
#define  EST	   2
#define  WEST	   3

/***************** DEFINITION MESSAGES ***********************/

#define  LOCAL     120
#define  REMOTE	   130



/*************** DEFINITIONS OF SOURCE AND DESTINATION NODES ***************/



struct point
{
  int  x;
  int  y;
};



struct message
{
  struct point		source;
  struct point		destination;
  double   		generation_time;
  int      		num_flits;
  int     		msg_id;
  int			wraparound;
  struct point		sender;
};




struct buffer
{
 int			status;
 int			msg_id;
 struct point		receiver;
 struct point		sender;
 int			local;
 int			flits_remaining;
 int			flit_type;
 int			ack;
 int			flit_status;
 int			token;
};




struct node
{
 struct message 	     queue[NODE_QUEUE_LENGTH];
 struct message 	     queue_wraparound[NODE_QUEUE_LENGTH];
 int			     last_in_queue;
 int			     last_in_queue_wraparound;
 struct buffer		     buffers[NUM_DIRECTIONS][NUM_VIRTUAL_CHANNELS_PER_DIRECTION];
 int			     physical_channels_status[NUM_DIRECTIONS];
};





