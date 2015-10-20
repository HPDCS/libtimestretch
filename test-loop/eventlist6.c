
//#include "def6.h"
#include <stdio.h>
#include <unistd.h>
#include "pcs.h"



typedef event msg_type;

#ifdef OLD_EVENTS

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

#endif


event  event_list[EV_LIST_LENGTH];

int             event_list_first;
int             event_list_last;
int 		max_event_list_length;


/**************************** EVENT LIST INITIALIZATION  ***************/

void  initialize_event_list()
{
	event_list_first      = -1;
	event_list_last       = -1;
	max_event_list_length = -1;
}


/********************* PROCEDURA INSERIMENTO EVENTI *********************/


void insert(new_event)
	struct event new_event;
{
  int  aux,temp;


/*
#ifdef DEBUG_SCHEDULING_SIMPLE
  printf("\nINSERIMENTO IN CODA SCHEDULING\n");

#endif
*/

  if(event_list_last == (EV_LIST_LENGTH - 1))
  {
    printf("lista eventi satura - ERROR2\n");
    exit(-1);
  }
  else
  {

/*
#ifdef DEBUG_SCHEDULING_SIMPLE 
    printf("     INSERIMENTO EVENTO - destinatario (%d,%d) - event type %d con tempo %e\n", new_event.which.x, new_event.which.x, new_event.type, new_event.time);     
#endif
*/
    if((event_list_last == -1) && (event_list_first == -1))
    {
     event_list_last = 0;
     event_list_first = 0;
     event_list[0] = new_event;
    }
    else
    {

         temp = event_list_last;

         if(event_list[temp].time <= new_event.time)
         {
	  temp += 1;
	  event_list[temp] = new_event;
	  event_list_last = temp;
         }
         else
         {
         
	   while((temp >= 0) && (event_list[temp].time > new_event.time)) temp -= 1;
           for(aux=event_list_last; aux>temp; aux--)  event_list[(aux+1)] = event_list[aux];
	   event_list[(temp+1)] = new_event; 
	   event_list_last += 1;
             
          } 
 
     }
  }

#ifdef DEBUG_SCHEDULING 
    printf("   EVENT INSERTED\n");     
#endif


#ifdef  DEBUG_SCHEDULING

	printf("     CODA DI SCHEDULING\n");
	for(temp = event_list_first;temp <= event_list_last; temp++)
 	{ printf("    NODE (%d,%d) --  EVENT TYPE %d  at time = %e\n",event_list[temp].which.x,event_list[temp].which.y, event_list[temp].type ,event_list[temp].time); }

#endif


if(max_event_list_length < event_list_last)
{
	max_event_list_length = event_list_last;
}

}  /* END */

/*******************  SCHEDULAZIONE PROSSIMO EVENTO **********************/



void next(struct event *current_event)

{
     int  i;


	if(event_list_first == -1)
	{
	  printf("lista degli eventi vuota - ERRORE1\n");
	  exit(-1);
	}
	else
	{

          *current_event = event_list[event_list_first];

	  if(event_list_first == event_list_last)              /* UN SOLO EVENTO IN LISTA */
	   {
	    event_list_first = -1;
	    event_list_last  = -1;
	   }
	   else
	   {
       	        for(i=event_list_first;i<event_list_last;i++)          event_list[i] = event_list[(i+1)];

                event_list_last -= 1;
	   }
	}

}
/*
	if(current_event.ty == SCHEDULING)
	{
        printf("\n   PROCESSO %d -- SCHEDULING al tempo = %e\n",current_event.pid,current_event.real_time);
	}
	else  { printf("\n	PROCESSO %d -- END EVENT al tempo = %e\n",current_event.pid,current_event.real_time);}
*/

/*
        printf("	STATO FINALE DELLA CODA DI SCHEDULING\n");

	  printf("   CODA DI SCHEDULING - prima posizione %d\n",event_list_first);
	  
	  for(i=event_list_first;(i<=event_list_last) && (i != -1);i++)
	  if(event_list[i].ty == SCHEDULING)
	  {
            printf("       PROCESSO %d -- SCHEDULING al tempo = %e\n",event_list[i].pid,event_list[i].real_time);
	  }
	  else  { printf("      PROCESSO %d -- END EVENT al tempo = %e\n",event_list[i].pid,event_list[i].real_time); }

*/



void event_list_statistics()
{
	printf("\nMAX EVENT LIST LENGTH = %d\n", max_event_list_length);
}


