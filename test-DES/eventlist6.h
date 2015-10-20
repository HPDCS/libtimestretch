
struct event
{
  double 			time;
  int    			type;
  struct point   		which;   /* IDENTITY OF THE DESTINATION OF THE EVENT */  
  struct point   		from;    /* IDENTITY OF THE SCHEDULING PROCESS */
  int				msg_id;
  int				direction;
  int				flit_type;
};

extern struct event  event_list[];

extern int             event_list_first;
extern int             event_list_last;
extern int 		max_event_list_length;


/**************************** EVENT LIST INITIALIZATION  ***************/

void  initialize_event_list();


/********************* PROCEDURA INSERIMENTO EVENTI *********************/


void insert(struct event new_event);

/*******************  SCHEDULAZIONE PROSSIMO EVENTO **********************/

void next(struct event *current_event);

void event_list_statistics();

