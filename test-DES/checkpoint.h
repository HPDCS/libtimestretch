#ifndef _CHECKPOINT_H
#define _CHECKPOINT_H

/* errors */
#define TOO_MANY  (-10)
#define ALREADY_PRUNED  (-11)

typedef double timestamp;

int checkpoint_init();
int checkpoint_close();
int dump_check_data();

int log_state(timestamp lvt);
int recover_state(timestamp lvt);
int prune_up_to(timestamp lvt);

//int check_loggedrecords (int lp);

#endif /* _CHECKPOINT_H */
