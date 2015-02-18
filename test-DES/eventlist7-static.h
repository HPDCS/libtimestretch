#ifndef _EVENTLIST6_H
#define _EVENTLIST6_H


#ifdef VECCHIO_CONTENUTO
struct event_content
{
  double 			time;
  int    			type;
  struct point   		which;   /* IDENTITY OF THE DESTINATION OF THE EVENT */  
  struct point   		from;    /* IDENTITY OF THE SCHEDULING PROCESS */
  int				msg_id;
  int				direction;
  int				flit_type;
};
#endif


struct event
{
  int valid;
  struct event * next;
  struct event * prev;
  msg_type single_event;
};

//extern struct event  event_list[];

//extern int             event_list_first;
//extern int             event_list_last;
//extern int 		max_event_list_length;


/**************************** EVENT LIST INITIALIZATION  ***************/

void  initialize_event_list();


/********************* PROCEDURA INSERIMENTO EVENTI *********************/


void insert(msg_type new_event);

/*******************  SCHEDULAZIONE PROSSIMO EVENTO **********************/

void next(msg_type *current_event);

void event_list_statistics();

#endif /* _EVENTLIST6_H */
