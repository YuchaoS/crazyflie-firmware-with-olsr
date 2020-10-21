#define DEBUG_MODULE "OLSR"

#include "olsrStruct.h"
#include <semphr.h>

#include "mac.h"
#include "olsrDebug.h"
xQueueHandle olsr_send_queue;
xQueueHandle olsr_recv_queue;
xSemaphoreHandle olsr_seq_mutex;
void olsr_send_queue_init(){
    olsr_send_queue = xQueueCreate(10,sizeof(packet_t));
    DEBUG_PRINT_OLSR_SYSTEM("SEND_QUEUE_INIT_SUCCESSFUL\n");
}
void olsr_recv_queue_init(){
    olsr_recv_queue = xQueueCreate(10,sizeof(packet_t));
    DEBUG_PRINT_OLSR_SYSTEM("RECV_QUEUE_INIT_SUCCESSFUL\n");
}
void olsr_seq_mutex_init(){
    olsr_seq_mutex = xSemaphoreCreateMutex();
}
