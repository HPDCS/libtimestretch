
#define CKPT		(5)
#define RECOVER		(6)
#define CKPT_RECOVER	(7)

int RTI_init();
void RTI_stub (int action, double lvt);
int RTI_prune(double lvt);
void RTI_print_data();
