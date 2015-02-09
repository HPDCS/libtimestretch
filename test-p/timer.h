#ifndef _TIMER_H_
#define _TIMER_H_

#define DECLARE_TIMER(timer_name) struct timeval timer_name;
#define TIMER_START(timer_name) gettimeofday(&timer_name, NULL);
#define TIMER_RESTART(timer_name) gettimeofday(&timer_name, NULL);

#define TIMER_VALUE(timedif,timer_name) struct timeval tmp_##timer_name;\
                                        int timedif;\
                                        gettimeofday(&tmp_##timer_name, NULL);\
                                        timedif = tmp_##timer_name.tv_sec * 1000 + tmp_##timer_name.tv_usec / 1000;\
                                        timedif -= timer_name.tv_sec * 1000 + timer_name.tv_usec / 1000;

#define TIMER_VALUE_MICRO(timedif,timer_name) struct timeval tmp_##timer_name;\
                                        int timedif;\
                                        gettimeofday(&tmp_##timer_name, NULL);\
                                        timedif = tmp_##timer_name.tv_sec * 1000000 + tmp_##timer_name.tv_usec;\
                                        timedif -= timer_name.tv_sec * 1000000 + timer_name.tv_usec;

#define TIMER_PRINT(timedif) printf("timer is %d\n", timedif);

#endif

