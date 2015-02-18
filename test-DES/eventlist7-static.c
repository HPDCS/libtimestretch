
//#include "def6.h"
#include <stdio.h>
#include <unistd.h>
#include "events.h"
#include "eventlist7-static.h"
#include "log.h"

void exit(int);

struct event  event_list[EV_LIST_LENGTH];
unsigned int  free_slot_stack[EV_LIST_LENGTH];
int top_stack;

struct event *  event_list_first;
struct event *  event_list_last;
int 		max_event_list_length;
int 		event_list_length;



typedef struct _list_log_buffer {
    struct event  event_list[EV_LIST_LENGTH];
    unsigned int  free_slot_stack[EV_LIST_LENGTH];
    int top_stack;

    struct event *  event_list_first;
    struct event *  event_list_last;
    int             max_event_list_length;
    int             event_list_length;
} list_log_buffer;


typedef struct _list_log_entry{
    double timestamp;
    list_log_buffer list_buffer;
} list_log_entry;


struct _list_log {
    int first;
    int last;
    int free_entries;
    list_log_entry list_log_array[MAX_LIST_LOGS];
} list_log;


void list_log_init(void){
    list_log.first = -1;
    list_log.last = -1;
    list_log.free_entries = MAX_LIST_LOGS;
}


void log_list(double timestamp){

    int i;

    if (list_log.free_entries == 0){
	printf("log list error: no more room on list log!\n");
	exit(-1);
    }
    
    if (list_log.free_entries == MAX_LIST_LOGS){
	list_log.first = 0;
	list_log.last = 0;
    }
    else{
	list_log.last++;
	if(list_log.last == MAX_LIST_LOGS) list_log.last = 0;
    }

    list_log.list_log_array[list_log.last].timestamp = timestamp;


    for (i=0; i< EV_LIST_LENGTH; i++){
	list_log.list_log_array[list_log.last].list_buffer.event_list[i] = event_list[i];
	list_log.list_log_array[list_log.last].list_buffer.free_slot_stack[i] = free_slot_stack[i];
    }

//    memcpy((char*)(list_log.list_log_array[list_log.last].list_buffer.event_list),(char*)event_list,sizeof((struct event)*EV_LIST_LENGTH));
    //  memcpy((char*)list_log.list_log_array[list_log.last].list_buffer.free_slot_stack,(char*)free_slot_stack,sizeof((unsigned int)*EV_LIST_LENGTH));

    list_log.list_log_array[list_log.last].list_buffer.top_stack  = top_stack;
    
    list_log.list_log_array[list_log.last].list_buffer.event_list_first = event_list_first;
    list_log.list_log_array[list_log.last].list_buffer.event_list_last = event_list_last;
    list_log.list_log_array[list_log.last].list_buffer.max_event_list_length = max_event_list_length;
    list_log.list_log_array[list_log.last].list_buffer.event_list_length =  event_list_length;
    
    list_log.free_entries--;

}



void prune_list_log(double prunetime){

    if( (list_log.first != -1) && (list_log.last != -1) ){ 
	while(1){
	    if( list_log.list_log_array[list_log.first].timestamp < prunetime){
		list_log.first++;
		if(list_log.first == MAX_LIST_LOGS) list_log.first = 0;
		list_log.free_entries++;
		if (list_log.free_entries == MAX_LIST_LOGS){
		    list_log.first = 0;
		    list_log.last = 0;
		    goto out;
		}
	    }
	    else{
		goto out;    
	    }
	}
    }    
 out: return;

}

/*
int consistent()
{
int temp;
int counter;
	
if ((event_list_last == -1) || (event_list_first == -1))
  {
    if (event_list_last != -1) {
      printf ("Inconsistent event_list_last, should be -1 is %d\n", event_list_last);
      return 0;
    }
    if (event_list_first != -1) {
      printf ("Inconsistent event_list_first, should be -1 is %d\n", event_list_first);
      return 0;
    }
    if (event_list_length != 0) {
      printf ("Inconsistent event_list_length, should be 0 is %d\n", event_list_length);
      return 0;
    }
  }

if (event_list_last == event_list_first) {
    if (event_list_length != 1) {
      printf ("Inconsistent event_list_length, should be 1 is %d\n", event_list_length);
      return 0;
    }
}
counter = 0;
temp = event_list_last;
while (temp >= 0)
  {
  //printf("-%d", temp);
  //printf("-%d:%f", event_list[temp].single_event.receiver, event_list[temp].single_event.time);
  if ((event_list[temp].prev >= 0) && (event_list[event_list[temp].prev].next != temp)) {
	  printf("Broken link prev %d next %d!\n", temp, event_list[event_list[temp].prev].next);
	  return 0;
  }
  temp = event_list[temp].prev;
  counter++;
  }
printf("-\n");

if (counter != event_list_length) {
      printf ("Inconsistent event_list_length, should be %d is %d\n", counter, event_list_length);
      return 0;
  }
return 1;
}
*/

int find_free_slot()
{
unsigned int temp;
	
temp = free_slot_stack[top_stack];
top_stack--;
if ((event_list[temp].valid))
  {
  printf("Allocating a valid slot!!!\n");
  exit(-1);
  }
return (temp);

/*

  static int last_slot = 0;
  int i;
  
  //temp = first_free_slot;
  for (i=last_slot + 1; i<EV_LIST_LENGTH; i++)
	  if (!(event_list[i].valid))
	    {
	    last_slot = i;
            return (last_slot);
	    }
  for (i=0; i<last_slot; i++)
	  if (!(event_list[i].valid))
	    {
	    last_slot = i;
            return (last_slot);
	    }
  return -1;
*/
}

//#define release_slot(X)
void release_slot(unsigned int slot)
{

if ((event_list[slot].valid))
  {
  printf("Releasing a valid slot!!!\n");
  exit(-1);
  }
top_stack++;
free_slot_stack[top_stack] = slot;
return;
}

void create_stack()
{

  int i;

  for (i = 0; i < EV_LIST_LENGTH; i++) free_slot_stack[i] = i;
  top_stack = EV_LIST_LENGTH - 1;
  return;

}

/**************************** EVENT LIST INITIALIZATION  ***************/

void  initialize_event_list()
{
	int i;

	event_list_first      = NULL;
	event_list_last       = NULL;
	max_event_list_length = -1;
	event_list_length = 0;
	for (i=0; i< EV_LIST_LENGTH; i++) event_list[i].valid = 0;
	create_stack();
}


/********************* PROCEDURA INSERIMENTO EVENTI *********************/


void insert(new_event)
	msg_type new_event;
{
  int temp;
//  int temp2;
  struct event * temp_pointer;
  struct event * temp_pointer2;


/*
#ifdef DEBUG_SCHEDULING_SIMPLE
  printf("\nINSERIMENTO IN CODA SCHEDULING\n");

#endif
*/
/*
  if(event_list_last == (EV_LIST_LENGTH - 1))
  {
    printf("lista eventi satura - ERROR2\n");
    exit(-1);
  }
  else
  {
*/
/*
#ifdef DEBUG_SCHEDULING_SIMPLE 
    printf("     INSERIMENTO EVENTO - destinatario (%d,%d) - event type %d con tempo %e\n", new_event.which.x, new_event.which.x, new_event.type, new_event.time);     
#endif
*/
    if((event_list_last == NULL) && (event_list_first == NULL))
    {
	temp = find_free_slot();
	 //printf ("Inserting 0 (init)\n");
     //temp_pointer = &(event_list[0]);
     temp_pointer = &(event_list[temp]);
     event_list_last = temp_pointer;
     event_list_first = temp_pointer;
     temp_pointer->single_event = new_event;
     temp_pointer->next = NULL;
     temp_pointer->prev = NULL;
     temp_pointer->valid = 1;
    }
    else
    {

	 temp = find_free_slot();
	 if (temp == -1) 
	  {
	    printf("lista eventi satura - ERROR2\n");
	    exit(-1);
	  }
	 temp_pointer = &(event_list[temp]);
	 if (temp_pointer->valid)
	  {
	    printf("valid slot assumed empty\n");
	    exit(-1);
	  }
	 //printf ("Inserting %i\n", temp);
	 temp_pointer->single_event = new_event;
         if(event_list_last->single_event.timestamp <= new_event.timestamp)
         {
	 //printf ("A\n");
	  //temp += 1;
	  temp_pointer->next = NULL;
	  temp_pointer->prev = event_list_last;
	  temp_pointer->valid = 1;
	  event_list_last->next = temp_pointer;
	  event_list_last = temp_pointer;
         }
         else
         {
         
	 //printf ("B\n");
           temp_pointer2 = event_list_last;
	   while((temp_pointer2 != NULL) && (temp_pointer2->single_event.timestamp > new_event.timestamp))
	   {
		   //temp_pointer2 = &(event_list[temp2]);
		   if (!(temp_pointer2->valid)) printf("Invalid record in list!!!\n");
		   temp_pointer2 = temp_pointer2->prev;
	   }
           //for(aux=event_list_last; aux>temp; aux--)  event_list[(aux+1)] = event_list[aux];
	   temp_pointer->prev = temp_pointer2; 
	   if (temp_pointer2 == NULL)
	   {
	 //printf ("C\n");
	     temp_pointer->next = event_list_first;
	     event_list_first = temp_pointer;
	   } else
	   {
	 //printf ("D %d, lvt2 %f, lvt %f\n", temp2, event_list[temp2].single_event.time, new_event.time);
	     temp_pointer->next = temp_pointer2->next; 
	     temp_pointer2->next = temp_pointer; 
	 //printf ("event_list.next %d\n", event_list[temp].next);
	   }
    //printf("Y3\n");fflush(stdout);     
    //printf("%x\n", ((unsigned int)temp_pointer));fflush(stdout);     
    //printf(" %x\n", (unsigned int)(temp_pointer->next));fflush(stdout);     
    //printf("  %x\n", (unsigned int)(temp_pointer->next));fflush(stdout);     
	   temp_pointer->next->prev = temp_pointer; 
	   temp_pointer->valid = 1;
             
         } 
 
     }
     event_list_length++;
     //printf("Insert: ");
     //if (!consistent()) {printf ("Inconsistent!\n"); exit(-1);}
/*
  }
*/
#ifdef DEBUG_SCHEDULING 
    printf("   EVENT INSERTED\n");     
#endif


#ifdef  DEBUG_SCHEDULING

	printf("     CODA DI SCHEDULING\n");
	for(temp = event_list_first;temp <= event_list_last; temp++)
 	{ printf("    NODE (%d,%d) --  EVENT TYPE %d  at time = %e\n",event_list[temp].which.x,event_list[temp].which.y, event_list[temp].type ,event_list[temp].time); }

#endif


//if(max_event_list_length < event_list_last)
//{
//	max_event_list_length = event_list_last;
//}

}  /* END */

/*******************  SCHEDULAZIONE PROSSIMO EVENTO **********************/



void next(msg_type *current_event)

{
     int  i;
     struct event * temp;


	if(event_list_first == NULL)
	{
	  printf("lista degli eventi vuota - ERRORE1\n");
	  exit(-1);
	}
	else
	{

          *current_event = event_list_first->single_event;
	  temp = event_list_first;
	  //printf ("Releasing %i\n", temp);
	  if(event_list_first == event_list_last)              /* UN SOLO EVENTO IN LISTA */
	   {
	  //printf("lista degli eventi svuotata %d (should hold %d)\n", event_list_last, event_list_length);
	    event_list_first = NULL;
	    event_list_last  = NULL;
	    temp->prev = NULL;
	    temp->next = NULL;
	   }
	   else
	   {
		temp->next->prev = NULL;
                event_list_first = temp->next;
		temp->prev = NULL;
		temp->next = NULL;
	   }
	  temp->valid = 0;
	  release_slot(temp - event_list);
	}
     event_list_length--;
     //printf("Next: ");
     //if (!consistent()) {printf ("Inconsistent!\n"); exit(-1);}

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
//	printf("\nMAX EVENT LIST LENGTH = %d\n", max_event_list_length);
}

void checklist()
{
int i, dummy;

for (i=0; i < EV_LIST_LENGTH; i++)
  {
  dummy = event_list[i].valid;
  }
}
