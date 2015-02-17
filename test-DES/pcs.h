

#define      MAX_NUM_CELLS  10000


#define      NUM_CELLS      10000
#define      EVENT_BOUND    10



/* DISTRIBUZIONI TIMESTAMP */
#define UNIFORME        0
#define ESPONENZIALE    1




#define  DISTRIBUZIONE_DURATA ESPONENZIALE
#define TA_DURATA            120.0

#define DISTRIBUZIONE_CAMBIOCELLA ESPONENZIALE
#define TA_CAMBIO            600.0


#define DISTRIBUZIONE ESPONENZIALE
#define TA            2.0





/* EVENT TYPES - PCS */
#define START_CALL      21
#define END_CALL        17
#define HANDOFF_LEAVE   18
#define HANDOFF_RECV    20



typedef double time_type;
typedef unsigned long seed_type;




typedef struct _msg_type{

  int   sender;
  int   receiver;
  int   type;

  time_type   send_time;
  time_type   timestamp;

  int cell;
  int channel;
  time_type   call_term_time;

} msg_type;



#define   CHANNELS_PER_CELL    100


/* Channel states */
#define CHAN_BUSY 1
#define CHAN_FREE 0


typedef  struct _sir_data_per_cell{
  int flag;
  //double distance;
  double fading;
  double power;
} sir_data_per_cell;



typedef struct _pcs_info{
  int    contatore_canali;
  double cont_chiamate_entranti;
  double cont_chiamate_complete;
  double cont_bloccate_in_partenza;
  double cont_bloccate_in_handoff;
  double cont_handoff_uscita;
  
  int            channel_state[CHANNELS_PER_CELL];
  double          xpower[CHANNELS_PER_CELL];
  double          xdistance[CHANNELS_PER_CELL];
  double          xcall_termination_time[CHANNELS_PER_CELL];
  double          xcall_start_time[CHANNELS_PER_CELL];
  sir_data_per_cell sir_data[CHANNELS_PER_CELL];
  
} pcs_info;


typedef struct _state {

  time_type lvt;
  //  int       queue;
  //  int       flag;
  pcs_info  pcs;

} state_type;



typedef struct __PCS_routing{
  int num_adjacent;
  int adjacent_identities[6];
} _PCS_routing;

 
