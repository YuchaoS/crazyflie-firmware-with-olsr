#ifndef __OLSR_ALGO_H__
#define __OLSR_ALGO_H__

#include "olsrPacket.h"

// void olsr_hello_task(void *ptr);



void olsrHelloTask(void *ptr);
void olsr_send_task(void *ptr);
void olsr_recv_task(void *ptr);
void olsr_tc_task(void *ptr);
void olsr_ts_task(void *ptr);
void queue_mutex_init();
void device_init(dwDevice_t *dev);
void olsrRxCallback(dwDevice_t *dev);
void olsr_send_message(olsrMessage_t* message);
void olsr_send_message_(olsrMessage_t* message);
void olsr_hello_send();
void olsr_generate_hello(olsrMessage_t *hello_message);
void olsr_message_append(olsrMessage_t* msg, void* data, int size_bytes);
void olsr_process_hello_message(uint8_t* message);
void olsr_increment_ansn();
#endif //__OLSR_ALGO_H__
