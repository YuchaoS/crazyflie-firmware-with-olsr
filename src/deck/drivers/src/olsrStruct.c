#define DEBUG_MODULE "OLSR"

#include "olsrStruct.h"
#include <semphr.h>

#include "mac.h"
#include "olsrDebug.h"

/*
************************QueueInitFunctions********************
*/
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

/*
************************DummyNode********************
*/
#define FREE_ENTRY 0
#define FULL_ENTRY 1
static setIndex_t olsrSetIndexEntry[OLSR_SETS_NUM][2]; 
/*
************************TopologySetFunctions********************
*/
static SemaphoreHandle_t olsrTopologyEmptySetLock;
static SemaphoreHandle_t olsrTopologyFullSetLock;


static void olsrTopologySetInit()
{
  int i;
  for(i=0; i < TOPOLOGY_SET_SIZE-1; i++)
    {
      olsrTopologySet[i].next = i+1;
    }
  olsrTopologySet[i].next = -1;
  olsrSetIndexEntry[TOPOLOGY_SET_T][FREE_ENTRY] = 0;
  olsrSetIndexEntry[TOPOLOGY_SET_T][FULL_ENTRY] = -1;
}

static setIndex_t olsrTopologySetMalloc()
{
  xSemaphoreTake(olsrTopologyEmptySetLock, portMAX_DELAY);
  if(olsrSetIndexEntry[TOPOLOGY_SET_T][FREE_ENTRY]==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      xSemaphoreGive(olsrTopologyEmptySetLock);
      return -1;
    }
  else
    { 
      setIndex_t candidate = olsrSetIndexEntry[TOPOLOGY_SET_T][FREE_ENTRY];
      olsrSetIndexEntry[TOPOLOGY_SET_T][FREE_ENTRY] = olsrTopologySet[candidate].next;
      xSemaphoreGive(olsrTopologyEmptySetLock);
      //insert to full queue
      xSemaphoreTake(olsrTopologyFullSetLock, portMAX_DELAY);
      setIndex_t tmp = olsrSetIndexEntry[TOPOLOGY_SET_T][FULL_ENTRY];
      olsrSetIndexEntry[TOPOLOGY_SET_T][FULL_ENTRY] = candidate;
      olsrTopologySet[candidate].next = tmp;
      xSemaphoreGive(olsrTopologyFullSetLock);
      return candidate;
    }
}

static bool olsrTopologySetFree(setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  xSemaphoreTake(olsrTopologyFullSetLock, portMAX_DELAY);
  setIndex_t pre = olsrSetIndexEntry[TOPOLOGY_SET_T][FULL_ENTRY];
  if(delItem == pre)
    {
      olsrSetIndexEntry[TOPOLOGY_SET_T][FULL_ENTRY] = olsrTopologySet[pre].next;
      xSemaphoreGive(olsrTopologyFullSetLock);
      //insert to empty queue
      xSemaphoreTake(olsrTopologyEmptySetLock,portMAX_DELAY);
      olsrSetIndexEntry[TOPOLOGY_SET_T][FULL_ENTRY] = delItem;
      olsrTopologySet[delItem].next = pre;
      xSemaphoreGive(olsrTopologyEmptySetLock);
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(olsrTopologySet[pre].next==delItem)
            {
              olsrTopologySet[pre].next = olsrTopologySet[delItem].next;
              xSemaphoreGive(olsrTopologyFullSetLock);
              //insert to empty queue
              xSemaphoreTake(olsrTopologyEmptySetLock,portMAX_DELAY);
              olsrSetIndexEntry[TOPOLOGY_SET_T][FULL_ENTRY] = delItem;
              olsrTopologySet[delItem].next = pre;
              xSemaphoreGive(olsrTopologyEmptySetLock);
              return true;
            }
          pre = olsrTopologySet[pre].next;
        }
    }
    return false;
}

bool olsrInsertToTopologySet(olsrTopologyTuple_t *tcTuple)
{
  if(tcTuple==NULL) return false;
  setIndex_t candidate = olsrTopologySetMalloc(); 
  if(candidate == -1) return false;
  olsrTopologySet[candidate].data.m_destAddr = tcTuple->m_destAddr;
  olsrTopologySet[candidate].data.m_distance = tcTuple->m_distance;
  olsrTopologySet[candidate].data.m_expirationTime = tcTuple->m_expirationTime;
  olsrTopologySet[candidate].data.m_lastAddr = tcTuple->m_lastAddr;
  return true;
}

void olsrPrintTopologySet()
{
  setIndex_t pre = olsrSetIndexEntry[TOPOLOGY_SET_T][FULL_ENTRY];
  while(pre!=-1)
    {
      DEBUG_PRINT_OLSR_SET("%d : destAddr:%u lastAddr:%u distance:%d\n",pre,\
                            olsrTopologySet[pre].data.m_destAddr,\
                            olsrTopologySet[pre].data.m_lastAddr,\
                            olsrTopologySet[pre].data.m_distance);
      pre =  olsrTopologySet[pre].next;
    }
}
static void olsrSetEntryInit()
{
  olsrTopologyEmptySetLock = xSemaphoreCreateMutex();
  olsrTopologyFullSetLock = xSemaphoreCreateMutex();
}


void olsrStructInitAll(dwDevice_t *dev)
{
    olsrSendQueueInit();
    olsrRecvQueueInit();
    olsrSetEntryInit();
    olsrTopologySetInit();

}
