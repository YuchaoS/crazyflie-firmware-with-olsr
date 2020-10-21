#ifndef __OLSR_STRUCT_H__
#define __OLSR_STRUCT_H__
#include "FreeRTOS.h"
#include<queue.h>
void olsr_send_queue_init();
void olsr_recv_queue_init();
void olsr_seq_mutex_init();
#endif //__OLSR_STRUCT_H__