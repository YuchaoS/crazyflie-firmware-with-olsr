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
#ifdef ii
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
      olsrTopologySet[delItem].next = olsrSetIndexEntry[TOPOLOGY_SET_T][FREE_ENTRY];
      olsrSetIndexEntry[TOPOLOGY_SET_T][FREE_ENTRY] = delItem;
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
              olsrTopologySet[delItem].next = olsrSetIndexEntry[TOPOLOGY_SET_T][FREE_ENTRY];
              olsrSetIndexEntry[TOPOLOGY_SET_T][FREE_ENTRY] = delItem;
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
#endif
/*
************************LinkSetFunctions********************
*/

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
  // xSemaphoreTake(olsrLinkEmptySetLock, portMAX_DELAY);
  if(olsrSetIndexEntry[LINK_SET_T][FREE_ENTRY]==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      // xSemaphoreGive(olsrLinkEmptySetLock);
      return -1;
    }
  else
    { 
      setIndex_t candidate = olsrSetIndexEntry[LINK_SET_T][FREE_ENTRY];
      olsrSetIndexEntry[LINK_SET_T][FREE_ENTRY] = olsrLinkSet[candidate].next;
      // xSemaphoreGive(olsrLinkEmptySetLock);
      //insert to full queue
      // xSemaphoreTake(olsrLinkFullSetLock, portMAX_DELAY);
      setIndex_t tmp = olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY];
      olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY] = candidate;
      olsrLinkSet[candidate].next = tmp;
      // xSemaphoreGive(olsrLinkFullSetLock);
      return candidate;
    }
}

// static bool olsrLinkSetFree(setIndex_t delItem)
// {
//   if(-1==delItem) return true;
//   //del from full queue
//   // xSemaphoreTake(olsrLinkFullSetLock, portMAX_DELAY);
//   setIndex_t pre = olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY];
//   if(delItem == pre)
//     {
//       olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY] = olsrLinkSet[pre].next;
//       // xSemaphoreGive(olsrLinkFullSetLock);
//       //insert to empty queue
//       // xSemaphoreTake(olsrLinkEmptySetLock,portMAX_DELAY);
//       olsrLinkSet[delItem].next = olsrSetIndexEntry[LINK_SET_T][FREE_ENTRY];
//       olsrSetIndexEntry[LINK_SET_T][FREE_ENTRY] = delItem;
//       // xSemaphoreGive(olsrLinkEmptySetLock);
//       return true;
//     }
//   else 
//     {
//       while(pre!=-1)
//         {
//           if(olsrLinkSet[pre].next==delItem)
//             {
//               olsrLinkSet[pre].next = olsrLinkSet[delItem].next;
//               // xSemaphoreGive(olsrLinkFullSetLock);
//               //insert to empty queue
//               // xSemaphoreTake(olsrLinkEmptySetLock,portMAX_DELAY);
//               olsrLinkSet[delItem].next = olsrSetIndexEntry[LINK_SET_T][FREE_ENTRY];
//               olsrSetIndexEntry[LINK_SET_T][FREE_ENTRY] = delItem;
//               // xSemaphoreGive(olsrLinkEmptySetLock);
//               return true;
//             }
//           pre = olsrLinkSet[pre].next;
//         }
//     }
//     return false;
// }

setIndex_t olsrFindInLinkByAddr(const olsrAddr_t addr)
{
  setIndex_t it = olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY];
  while(it!=-1)
    {
      if(olsrLinkSet[it].data.m_neighborAddr==addr)
        {
          break;
        }
      it = olsrLinkSet[it].next;
    }
  return it;
}
setIndex_t olsrInsertToLinkSet(olsrLinkTuple_t *item)
{
  setIndex_t candidate = olsrLinkSetMalloc();
  if(candidate!=-1)
    {
      memcpy(&olsrLinkSet[candidate].data,item,sizeof(olsrLinkTuple_t));
    }
  else
    {
      DEBUG_PRINT_OLSR_LINK("bad alloc whan insert by function [olsrInsertToLinkSet]\n");
    }
  return candidate;
}
/*
************************NeighborSetFunctions********************
*/


static void olsrNeighborSetInit(olsrNeighborSetItem_t neighborObj[], setIndex_t *freeEntry, setIndex_t *fullEnrty)
{
  int i;
  for(i=0; i < NEIGHBOR_SET_SIZE-1; i++)
    {
      neighborObj[i].next = i+1;
    }
  neighborObj[i].next = -1;
  *freeEntry = 0;
  *fullEnrty = -1;
}

static setIndex_t olsrNeighborSetMalloc(olsrNeighborSetItem_t neighborObj[], setIndex_t *freeEntry, setIndex_t *fullEnrty)
{
  // xSemaphoreTake(olsrNeighborEmptySetLock, portMAX_DELAY);
  if(*freeEntry==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      // xSemaphoreGive(olsrNeighborEmptySetLock);
      return -1;
    }
  else
    { 
      setIndex_t candidate = *freeEntry;
      *freeEntry = neighborObj[candidate].next;
      // xSemaphoreGive(olsrNeighborEmptySetLock);
      //insert to full queue
      // xSemaphoreTake(olsrNeighborFullSetLock, portMAX_DELAY);
      setIndex_t tmp = *fullEnrty;
      *fullEnrty = candidate;
      neighborObj[candidate].next = tmp;
      // xSemaphoreGive(olsrNeighborFullSetLock);
      return candidate;
    }
}

static bool olsrNeighborSetFree(olsrNeighborSetItem_t neighborObj[], setIndex_t *freeEntry, setIndex_t *fullEnrty, setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  // xSemaphoreTake(olsrNeighborFullSetLock, portMAX_DELAY);
  setIndex_t pre = olsrSetIndexEntry[NEIGHBOR_SET_T][FULL_ENTRY];
  if(delItem == pre)
    {
      olsrSetIndexEntry[NEIGHBOR_SET_T][FULL_ENTRY] = olsrNeighborSet[pre].next;
      // xSemaphoreGive(olsrNeighborFullSetLock);
      //insert to empty queue
      // xSemaphoreTake(olsrNeighborEmptySetLock,portMAX_DELAY);
      olsrNeighborSet[delItem].next = olsrSetIndexEntry[NEIGHBOR_SET_T][FREE_ENTRY];
      olsrSetIndexEntry[NEIGHBOR_SET_T][FREE_ENTRY] = delItem;
      // xSemaphoreGive(olsrNeighborEmptySetLock);
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(olsrNeighborSet[pre].next==delItem)
            {
              olsrNeighborSet[pre].next = olsrNeighborSet[delItem].next;
              // xSemaphoreGive(olsrNeighborFullSetLock);
              //insert to empty queue
              // xSemaphoreTake(olsrNeighborEmptySetLock,portMAX_DELAY);
              olsrNeighborSet[delItem].next = olsrSetIndexEntry[NEIGHBOR_SET_T][FREE_ENTRY];
              olsrSetIndexEntry[NEIGHBOR_SET_T][FREE_ENTRY] = delItem;
              // xSemaphoreGive(olsrNeighborEmptySetLock);
              return true;
            }
          pre = olsrNeighborSet[pre].next;
        }
    }
    return false;
}

setIndex_t olsrFindNeighborByAddr(const olsrAddr_t addr)
{
  setIndex_t it = olsrSetIndexEntry[NEIGHBOR_SET_T][FULL_ENTRY];
  while(it!=-1)
    {
      if(olsrNeighborSet[it].data.m_neighborAddr==addr)
        {
          break;
        }
      it = olsrNeighborSet[it].next;
    }
  return it;
}
setIndex_t olsrInsertToNeighborSet(const olsrNeighborTuple_t* tuple)
{
  setIndex_t candidate = olsrNeighborSetMalloc();
  if(candidate != -1)
    {
      memcpy(&olsrNeighborSet[candidate].data,tuple,sizeof(olsrNeighborTuple_t));
    }
  else
    {
      DEBUG_PRINT_OLSR_NEIGHBOR("bad alloc whan insert by function [olsrInsertToNeighborSet]\n");
    }
  return candidate;
}

/*
************************TwoNeighborSetFunctions********************
*/


static void olsrTwoHopNeighborSetInit()
{
  int i;
  for(i=0; i < TWO_HOP_NEIGHBOR_SET_SIZE-1; i++)
    {
      olsrTwoHopNeighborSet[i].next = i+1;
    }
  olsrTwoHopNeighborSet[i].next = -1;
  olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FREE_ENTRY] = 0;
  olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FULL_ENTRY] = -1;
}

static setIndex_t olsrTwoHopNeighborSetMalloc()
{
  // xSemaphoreTake(olsrNeighborEmptySetLock, portMAX_DELAY);
  if(olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FREE_ENTRY]==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      // xSemaphoreGive(olsrNeighborEmptySetLock);
      return -1;
    }
  else
    { 
      setIndex_t candidate = olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FREE_ENTRY];
      olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FREE_ENTRY] = olsrNeighborSet[candidate].next;
      // xSemaphoreGive(olsrNeighborEmptySetLock);
      //insert to full queue
      // xSemaphoreTake(olsrNeighborFullSetLock, portMAX_DELAY);
      setIndex_t tmp = olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FULL_ENTRY];
      olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FULL_ENTRY] = candidate;
      olsrTwoHopNeighborSet[candidate].next = tmp;
      // xSemaphoreGive(olsrNeighborFullSetLock);
      return candidate;
    }
}

static bool olsrTwoHopNeighborSetFree(setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  // xSemaphoreTake(olsrNeighborFullSetLock, portMAX_DELAY);
  setIndex_t pre = olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FULL_ENTRY];
  if(delItem == pre)
    {
      olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FULL_ENTRY] = olsrTwoHopNeighborSet[pre].next;
      // xSemaphoreGive(olsrNeighborFullSetLock);
      //insert to empty queue
      // xSemaphoreTake(olsrNeighborEmptySetLock,portMAX_DELAY);
      olsrTwoHopNeighborSet[delItem].next = olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FREE_ENTRY];
      olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FREE_ENTRY] = delItem;
      // xSemaphoreGive(olsrNeighborEmptySetLock);
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(olsrTwoHopNeighborSet[pre].next==delItem)
            {
              olsrTwoHopNeighborSet[pre].next = olsrTwoHopNeighborSet[delItem].next;
              // xSemaphoreGive(olsrNeighborFullSetLock);
              //insert to empty queue
              // xSemaphoreTake(olsrNeighborEmptySetLock,portMAX_DELAY);
              olsrTwoHopNeighborSet[delItem].next = olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FREE_ENTRY];
              olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FREE_ENTRY] = delItem;
              // xSemaphoreGive(olsrNeighborEmptySetLock);
              return true;
            }
          pre = olsrTwoHopNeighborSet[pre].next;
        }
    }
    return false;
}
setIndex_t olsrFindTwoHopNeighborTuple(olsrAddr_t neighborAddr, olsrAddr_t twoHopNeighborAddr)
{
  setIndex_t candidate = olsrSetIndexEntry[TWO_HOP_NEIGHBOR_SET_T][FULL_ENTRY];
  while(candidate != -1)
    {
      if(olsrTwoHopNeighborSet[candidate].data.m_neighborAddr == neighborAddr\
      &&olsrTwoHopNeighborSet[candidate].data.m_twoHopNeighborAddr == twoHopNeighborAddr)
        {
          break;
        }
    }
  return candidate;
}

setIndex_t olsrInsertToTwoHopNeighborSet(const olsrTwoHopNeighborTuple_t* tuple)
{
  setIndex_t candidate = olsrTwoHopNeighborSetMalloc();
  if(candidate != -1)
    {
      memcpy(&olsrTwoHopNeighborSet[candidate].data,tuple,sizeof(olsrTwoHopNeighborTuple_t));
    }
  else
    {
      DEBUG_PRINT_OLSR_NEIGHBOR2("bad alloc.\n");
    }
  return candidate;
}

bool olsrEraseTwoHopNeighborTuple(olsrAddr_t neighborAddr, olsrAddr_t twoHopNeighborAddr)
{
  setIndex_t candidate = olsrFindTwoHopNeighborTuple(neighborAddr,twoHopNeighborAddr);
  if(candidate != -1)
    {
      if(olsrTwoHopNeighborSetFree(candidate) != false)
        {
          return true;
        }
    }
  return false;
}

/*
************************MprSetFunctions********************
*/

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

// static setIndex_t olsrMprSetMalloc()
// {
//   xSemaphoreTake(olsrMprEmptySetLock, portMAX_DELAY);
//   if(olsrSetIndexEntry[MPR_SET_T][FREE_ENTRY]==-1)
//     {
//       DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
//       xSemaphoreGive(olsrMprEmptySetLock);
//       return -1;
//     }
//   else
//     { 
//       setIndex_t candidate = olsrSetIndexEntry[MPR_SET_T][FREE_ENTRY];
//       olsrSetIndexEntry[MPR_SET_T][FREE_ENTRY] = olsrMprSet[candidate].next;
//       xSemaphoreGive(olsrMprEmptySetLock);
//       //insert to full queue
//       xSemaphoreTake(olsrMprFullSetLock, portMAX_DELAY);
//       setIndex_t tmp = olsrSetIndexEntry[MPR_SET_T][FULL_ENTRY];
//       olsrSetIndexEntry[MPR_SET_T][FULL_ENTRY] = candidate;
//       olsrMprSet[candidate].next = tmp;
//       xSemaphoreGive(olsrMprFullSetLock);
//       return candidate;
//     }
// }

// static bool olsrMprSetFree(setIndex_t delItem)
// {
//   if(-1==delItem) return true;
//   //del from full queue
//   xSemaphoreTake(olsrMprFullSetLock, portMAX_DELAY);
//   setIndex_t pre = olsrSetIndexEntry[MPR_SET_T][FULL_ENTRY];
//   if(delItem == pre)
//     {
//       olsrSetIndexEntry[MPR_SET_T][FULL_ENTRY] = olsrMprSet[pre].next;
//       xSemaphoreGive(olsrMprFullSetLock);
//       //insert to empty queue
//       xSemaphoreTake(olsrMprEmptySetLock,portMAX_DELAY);
//       olsrMprSet[delItem].next = olsrSetIndexEntry[MPR_SET_T][FREE_ENTRY];
//       olsrSetIndexEntry[MPR_SET_T][FREE_ENTRY] = delItem;
//       xSemaphoreGive(olsrMprEmptySetLock);
//       return true;
//     }
//   else 
//     {
//       while(pre!=-1)
//         {
//           if(olsrMprSet[pre].next==delItem)
//             {
//               olsrMprSet[pre].next = olsrMprSet[delItem].next;
//               xSemaphoreGive(olsrMprFullSetLock);
//               //insert to empty queue
//               xSemaphoreTake(olsrMprEmptySetLock,portMAX_DELAY);
//               olsrMprSet[delItem].next = olsrSetIndexEntry[MPR_SET_T][FREE_ENTRY];
//               olsrSetIndexEntry[MPR_SET_T][FREE_ENTRY] = delItem;
//               xSemaphoreGive(olsrMprEmptySetLock);
//               return true;
//             }
//           pre = olsrMprSet[pre].next;
//         }
//     }
//     return false;
// }

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
************************DuplicateSetFunctions********************
*/


static void olsrDuplicateSetInit()
{
  int i;
  for(i=0; i < DUPLICATE_SET_SIZE-1; i++)
    {
      olsrDuplicateSet[i].next = i+1;
    }
  olsrDuplicateSet[i].next = -1;
  olsrSetIndexEntry[DUPLICATE_SET_T][FREE_ENTRY] = 0;
  olsrSetIndexEntry[DUPLICATE_SET_T][FULL_ENTRY] = -1;
}
#ifdef p
static setIndex_t olsrDuplicateSetMalloc()
{
  xSemaphoreTake(olsrDuplicateEmptySetLock, portMAX_DELAY);
  if(olsrSetIndexEntry[DUPLICATE_SET_T][FREE_ENTRY]==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      xSemaphoreGive(olsrDuplicateEmptySetLock);
      return -1;
    }
  else
    { 
      setIndex_t candidate = olsrSetIndexEntry[DUPLICATE_SET_T][FREE_ENTRY];
      olsrSetIndexEntry[DUPLICATE_SET_T][FREE_ENTRY] = olsrDuplicateSet[candidate].next;
      xSemaphoreGive(olsrDuplicateEmptySetLock);
      //insert to full queue
      xSemaphoreTake(olsrDuplicateFullSetLock, portMAX_DELAY);
      setIndex_t tmp = olsrSetIndexEntry[DUPLICATE_SET_T][FULL_ENTRY];
      olsrSetIndexEntry[DUPLICATE_SET_T][FULL_ENTRY] = candidate;
      olsrDuplicateSet[candidate].next = tmp;
      xSemaphoreGive(olsrDuplicateFullSetLock);
      return candidate;
    }
}

static bool olsrDuplicateSetFree(setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  xSemaphoreTake(olsrDuplicateFullSetLock, portMAX_DELAY);
  setIndex_t pre = olsrSetIndexEntry[DUPLICATE_SET_T][FULL_ENTRY];
  if(delItem == pre)
    {
      olsrSetIndexEntry[DUPLICATE_SET_T][FULL_ENTRY] = olsrDuplicateSet[pre].next;
      xSemaphoreGive(olsrDuplicateFullSetLock);
      //insert to empty queue
      xSemaphoreTake(olsrDuplicateEmptySetLock,portMAX_DELAY);
      olsrDuplicateSet[delItem].next = olsrSetIndexEntry[DUPLICATE_SET_T][FREE_ENTRY];
      olsrSetIndexEntry[DUPLICATE_SET_T][FREE_ENTRY] = delItem;
      xSemaphoreGive(olsrDuplicateEmptySetLock);
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(olsrDuplicateSet[pre].next==delItem)
            {
              olsrDuplicateSet[pre].next = olsrDuplicateSet[delItem].next;
              xSemaphoreGive(olsrDuplicateFullSetLock);
              //insert to empty queue
              xSemaphoreTake(olsrDuplicateEmptySetLock,portMAX_DELAY);
              olsrDuplicateSet[delItem].next = olsrSetIndexEntry[DUPLICATE_SET_T][FREE_ENTRY];
              olsrSetIndexEntry[DUPLICATE_SET_T][FREE_ENTRY] = delItem;
              xSemaphoreGive(olsrDuplicateEmptySetLock);
              return true;
            }
          pre = olsrDuplicateSet[pre].next;
        }
    }
    return false;
}
#endif

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
  olsrTwoHopNeighborEmptySetLock = xSemaphoreCreateMutex();
  olsrTwoHopNeighborFullSetLock = xSemaphoreCreateMutex();
}


void olsrStructInitAll(dwDevice_t *dev)
{
  olsrSendQueueInit();
  olsrRecvQueueInit();
  olsrSetEntryInit();
  olsrTopologySetInit();
  olsrLinkSetInit();
  olsrNeighborSetInit();
  olsrTwoHopNeighborSetInit();
  olsrMprSetInit();
  olsrDuplicateSetInit();
}
