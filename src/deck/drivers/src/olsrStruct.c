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

/*
************************LinkSetFunctions********************
*/
static SemaphoreHandle_t olsrLinkEmptySetLock;
static SemaphoreHandle_t olsrLinkFullSetLock;

static void olsrLinkSetInit()
{
  int i;
  for(i=0; i < LINK_SET_SIZE-1; i++)
    {
      olsrLinkSet[i].next = i+1;
    }
  olsrLinkSet[i].next = -1;
  olsrSetIndexEntry[LINK_SET_T][FREE_ENTRY] = 0;
  olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY] = -1;
}

static setIndex_t olsrLinkSetMalloc()
{
  xSemaphoreTake(olsrLinkEmptySetLock, portMAX_DELAY);
  if(olsrSetIndexEntry[LINK_SET_T][FREE_ENTRY]==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      xSemaphoreGive(olsrLinkEmptySetLock);
      return -1;
    }
  else
    { 
      setIndex_t candidate = olsrSetIndexEntry[LINK_SET_T][FREE_ENTRY];
      olsrSetIndexEntry[LINK_SET_T][FREE_ENTRY] = olsrLinkSet[candidate].next;
      xSemaphoreGive(olsrLinkEmptySetLock);
      //insert to full queue
      xSemaphoreTake(olsrLinkFullSetLock, portMAX_DELAY);
      setIndex_t tmp = olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY];
      olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY] = candidate;
      olsrLinkSet[candidate].next = tmp;
      xSemaphoreGive(olsrLinkFullSetLock);
      return candidate;
    }
}

static bool olsrLinkSetFree(setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  xSemaphoreTake(olsrLinkFullSetLock, portMAX_DELAY);
  setIndex_t pre = olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY];
  if(delItem == pre)
    {
      olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY] = olsrLinkSet[pre].next;
      xSemaphoreGive(olsrLinkFullSetLock);
      //insert to empty queue
      xSemaphoreTake(olsrLinkEmptySetLock,portMAX_DELAY);
      olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY] = delItem;
      olsrLinkSet[delItem].next = pre;
      xSemaphoreGive(olsrLinkEmptySetLock);
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(olsrLinkSet[pre].next==delItem)
            {
              olsrLinkSet[pre].next = olsrLinkSet[delItem].next;
              xSemaphoreGive(olsrLinkFullSetLock);
              //insert to empty queue
              xSemaphoreTake(olsrLinkEmptySetLock,portMAX_DELAY);
              olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY] = delItem;
              olsrLinkSet[delItem].next = pre;
              xSemaphoreGive(olsrLinkEmptySetLock);
              return true;
            }
          pre = olsrLinkSet[pre].next;
        }
    }
    return false;
}

/*
************************NeighborSetFunctions********************
*/
static SemaphoreHandle_t olsrNeighborEmptySetLock;
static SemaphoreHandle_t olsrNeighborFullSetLock;

static void olsrNeighborSetInit()
{
  int i;
  for(i=0; i < NEIGHBOR_SET_SIZE-1; i++)
    {
      olsrNeighborSet[i].next = i+1;
    }
  olsrNeighborSet[i].next = -1;
  olsrSetIndexEntry[NEIGHBOR_SET_T][FREE_ENTRY] = 0;
  olsrSetIndexEntry[NEIGHBOR_SET_T][FULL_ENTRY] = -1;
}

static setIndex_t olsrNeighborSetMalloc()
{
  xSemaphoreTake(olsrNeighborEmptySetLock, portMAX_DELAY);
  if(olsrSetIndexEntry[NEIGHBOR_SET_T][FREE_ENTRY]==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      xSemaphoreGive(olsrNeighborEmptySetLock);
      return -1;
    }
  else
    { 
      setIndex_t candidate = olsrSetIndexEntry[NEIGHBOR_SET_T][FREE_ENTRY];
      olsrSetIndexEntry[NEIGHBOR_SET_T][FREE_ENTRY] = olsrNeighborSet[candidate].next;
      xSemaphoreGive(olsrNeighborEmptySetLock);
      //insert to full queue
      xSemaphoreTake(olsrNeighborFullSetLock, portMAX_DELAY);
      setIndex_t tmp = olsrSetIndexEntry[NEIGHBOR_SET_T][FULL_ENTRY];
      olsrSetIndexEntry[NEIGHBOR_SET_T][FULL_ENTRY] = candidate;
      olsrNeighborSet[candidate].next = tmp;
      xSemaphoreGive(olsrNeighborFullSetLock);
      return candidate;
    }
}

static bool olsrNeighborSetFree(setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  xSemaphoreTake(olsrNeighborFullSetLock, portMAX_DELAY);
  setIndex_t pre = olsrSetIndexEntry[NEIGHBOR_SET_T][FULL_ENTRY];
  if(delItem == pre)
    {
      olsrSetIndexEntry[NEIGHBOR_SET_T][FULL_ENTRY] = olsrNeighborSet[pre].next;
      xSemaphoreGive(olsrNeighborFullSetLock);
      //insert to empty queue
      xSemaphoreTake(olsrNeighborEmptySetLock,portMAX_DELAY);
      olsrSetIndexEntry[NEIGHBOR_SET_T][FULL_ENTRY] = delItem;
      olsrNeighborSet[delItem].next = pre;
      xSemaphoreGive(olsrNeighborEmptySetLock);
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(olsrNeighborSet[pre].next==delItem)
            {
              olsrNeighborSet[pre].next = olsrNeighborSet[delItem].next;
              xSemaphoreGive(olsrNeighborFullSetLock);
              //insert to empty queue
              xSemaphoreTake(olsrNeighborEmptySetLock,portMAX_DELAY);
              olsrSetIndexEntry[NEIGHBOR_SET_T][FULL_ENTRY] = delItem;
              olsrNeighborSet[delItem].next = pre;
              xSemaphoreGive(olsrNeighborEmptySetLock);
              return true;
            }
          pre = olsrNeighborSet[pre].next;
        }
    }
    return false;
}

/*
************************MprSetFunctions********************
*/
static SemaphoreHandle_t olsrMprEmptySetLock;
static SemaphoreHandle_t olsrMprFullSetLock;

static void olsrMprSetInit()
{
  int i;
  for(i=0; i < MPR_SET_SIZE-1; i++)
    {
      olsrMprSet[i].next = i+1;
    }
   olsrMprSet[i].next = -1;
  olsrSetIndexEntry[MPR_SET_T][FREE_ENTRY] = 0;
  olsrSetIndexEntry[MPR_SET_T][FULL_ENTRY] = -1;
}

static setIndex_t olsrMprSetMalloc()
{
  xSemaphoreTake(olsrMprEmptySetLock, portMAX_DELAY);
  if(olsrSetIndexEntry[MPR_SET_T][FREE_ENTRY]==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      xSemaphoreGive(olsrMprEmptySetLock);
      return -1;
    }
  else
    { 
      setIndex_t candidate = olsrSetIndexEntry[MPR_SET_T][FREE_ENTRY];
      olsrSetIndexEntry[MPR_SET_T][FREE_ENTRY] = olsrMprSet[candidate].next;
      xSemaphoreGive(olsrMprEmptySetLock);
      //insert to full queue
      xSemaphoreTake(olsrMprFullSetLock, portMAX_DELAY);
      setIndex_t tmp = olsrSetIndexEntry[MPR_SET_T][FULL_ENTRY];
      olsrSetIndexEntry[MPR_SET_T][FULL_ENTRY] = candidate;
      olsrMprSet[candidate].next = tmp;
      xSemaphoreGive(olsrMprFullSetLock);
      return candidate;
    }
}

static bool olsrMprSetFree(setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  xSemaphoreTake(olsrMprFullSetLock, portMAX_DELAY);
  setIndex_t pre = olsrSetIndexEntry[MPR_SET_T][FULL_ENTRY];
  if(delItem == pre)
    {
      olsrSetIndexEntry[MPR_SET_T][FULL_ENTRY] = olsrMprSet[pre].next;
      xSemaphoreGive(olsrMprFullSetLock);
      //insert to empty queue
      xSemaphoreTake(olsrMprEmptySetLock,portMAX_DELAY);
      olsrSetIndexEntry[MPR_SET_T][FULL_ENTRY] = delItem;
      olsrMprSet[delItem].next = pre;
      xSemaphoreGive(olsrMprEmptySetLock);
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(olsrMprSet[pre].next==delItem)
            {
              olsrMprSet[pre].next = olsrMprSet[delItem].next;
              xSemaphoreGive(olsrMprFullSetLock);
              //insert to empty queue
              xSemaphoreTake(olsrMprEmptySetLock,portMAX_DELAY);
              olsrSetIndexEntry[MPR_SET_T][FULL_ENTRY] = delItem;
              olsrMprSet[delItem].next = pre;
              xSemaphoreGive(olsrMprEmptySetLock);
              return true;
            }
          pre = olsrMprSet[pre].next;
        }
    }
    return false;
}

bool olsrFindMprByAddr(olsrAddr_t addr)
{
  bool isFound = false;
  setIndex_t it = olsrSetIndexEntry[MPR_SET_T][FULL_ENTRY];
  while(it!=-1)
    {
      if(olsrMprSet[it].data.m_addr==addr)
        {
          isFound = true;
          break;
        }
      it = olsrMprSet[it].next;
    }
  return isFound;
}




/*
************************CommonFunctions********************
*/












static void olsrSetEntryInit()
{
  olsrTopologyEmptySetLock = xSemaphoreCreateMutex();
  olsrTopologyFullSetLock = xSemaphoreCreateMutex();
  olsrLinkEmptySetLock = xSemaphoreCreateMutex();
  olsrLinkFullSetLock = xSemaphoreCreateMutex();
  olsrNeighborEmptySetLock = xSemaphoreCreateMutex();
  olsrNeighborFullSetLock = xSemaphoreCreateMutex();
  olsrMprEmptySetLock = xSemaphoreCreateMutex();
  olsrMprFullSetLock = xSemaphoreCreateMutex();
}


void olsrStructInitAll(dwDevice_t *dev)
{
    olsrSendQueueInit();
    olsrRecvQueueInit();
    olsrSetEntryInit();
    olsrTopologySetInit();
    olsrLinkSetInit();
    olsrNeighborSetInit();

}
