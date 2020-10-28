#define DEBUG_MODULE "OLSR"

#include "olsrStruct.h"
#include <semphr.h>

#include "mac.h"
#include "olsrDebug.h"

static void olsrSendQueueInit()
{
    g_olsrSendQueue = xQueueCreate(10,sizeof(packet_t));
    DEBUG_PRINT_OLSR_SYSTEM("SEND_QUEUE_INIT_SUCCESSFUL\n");
}
static void olsrRecvQueueInit()
{
    g_olsrRecvQueue = xQueueCreate(10,sizeof(packet_t));
    DEBUG_PRINT_OLSR_SYSTEM("RECV_QUEUE_INIT_SUCCESSFUL\n");
}
void olsrStructInitAll(dwDevice_t *dev)
{
    olsrSendQueueInit();
    olsrRecvQueueInit();

}
