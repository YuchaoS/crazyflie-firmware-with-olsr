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


static void olsrTopologySetInit(olsrTopologySet_t *topologySet)
{
  int i;
  for(i=0; i < TOPOLOGY_SET_SIZE-1; i++)
    {
      topologySet->setData[i].next = i+1;
    }
  topologySet->setData[i].next = -1;
  topologySet->freeQueueEntry = 0;
  topologySet->fullQueueEntry = -1;
}

static setIndex_t olsrTopologySetMalloc(olsrTopologySet_t *topologySet)
{
  if(topologySet->freeQueueEntry==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      return -1;
    }
  else
    { 
      setIndex_t candidate = topologySet->freeQueueEntry;
      topologySet->freeQueueEntry = topologySet->setData[candidate].next;
      //insert to full queue
      setIndex_t tmp = topologySet->fullQueueEntry;
      topologySet->fullQueueEntry = candidate;
      topologySet->setData[candidate].next = tmp;
      return candidate;
    }
}

static bool olsrTopologySetFree(olsrTopologySet_t *topologySet,\
                                setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  setIndex_t pre = topologySet->fullQueueEntry;
  if(delItem == pre)
    {
      topologySet->fullQueueEntry = topologySet->setData[pre].next;
      //insert to empty queue
      topologySet->setData[delItem].next = topologySet->freeQueueEntry;
      topologySet->freeQueueEntry = delItem;
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(topologySet->setData[pre].next==delItem)
            {
              topologySet->setData[pre].next = topologySet->setData[delItem].next;
              //insert to empty queue
              topologySet->setData[delItem].next = topologySet->freeQueueEntry;
              topologySet->freeQueueEntry = delItem;
              return true;
            }
          pre = topologySet->setData[pre].next;
        }
    }
    return false;
}

setIndex_t olsrInsertToTopologySet(olsrTopologySet_t *topologySet,\
                             const olsrTopologyTuple_t *tcTuple)
{
  setIndex_t candidate = olsrTopologySetMalloc(topologySet); 
  if(candidate != -1)
    {
      memcpy(&topologySet->setData[candidate].data,tcTuple,sizeof(olsrTopologyTuple_t));
    }
  else
    {
      DEBUG_PRINT_OLSR_TC("bad alloc in function[olsrInsertToTopologySet]\n");
    }
  return candidate;
}

void olsrPrintTopologySet(olsrTopologySet_t *topologySet)
{
  setIndex_t pre = topologySet->fullQueueEntry;
  while(pre!=-1)
    {
      DEBUG_PRINT_OLSR_SET("%d : destAddr:%u lastAddr:%u distance:%d\n",pre,\
                            topologySet->setData[pre].data.m_destAddr,\
                            topologySet->setData[pre].data.m_lastAddr,\
                            topologySet->setData[pre].data.m_distance);
      pre =  topologySet->setData[pre].next;
    }
}

setIndex_t olsrFindNewerTopologyTuple(olsrTopologySet_t *topologyset,\
                                      olsrAddr_t originator,\
                                      uint16_t ansn)
{
  setIndex_t candidate = topologyset->fullQueueEntry;
  while(candidate != -1)
    {
      olsrTopologySetItem_t item = topologyset->setData[candidate];
      if(item.data.m_lastAddr == originator && item.data.m_seqenceNumber > ansn)
        {
          break;
        }
      candidate = item.next;
    }
  return candidate;
}

void olsrEraseOlderTopologyTuples(olsrTopologySet_t *topologyset,\
                                  olsrAddr_t originator,\
                                  uint16_t ansn)
{
  setIndex_t it = topologyset->fullQueueEntry;
  while(it != -1)
    {
      olsrTopologySetItem_t item = topologyset->setData[it];
      if(item.data.m_lastAddr == originator && item.data.m_seqenceNumber < ansn)
        {
          setIndex_t del = it;
          it = item.next;
          olsrTopologySetFree(topologyset,del);
          continue;
        }
      it = item.next;
    }
}

setIndex_t olsrFindTopologyTuple(olsrTopologySet_t *topologyset,\
                                 olsrAddr_t destAddr,\
                                 olsrAddr_t lastAddr)
{
  setIndex_t candidate = topologyset->fullQueueEntry;
  while(candidate != -1)
    {
      olsrTopologySetItem_t item = topologyset->setData[candidate];
      if(item.data.m_lastAddr == lastAddr && item.data.m_destAddr == destAddr)
        {
          break;
        }
      candidate = item.next;
    }
  return candidate;
}
/*
************************LinkSetFunctions********************
*/

static void olsrLinkSetInit(olsrLinkSet_t *linkSet)
{
  int i;
  for(i=0; i < LINK_SET_SIZE-1; i++)
    {
      linkSet->setData[i].next = i+1;
    }
  linkSet->setData[i].next = -1;
  linkSet->freeQueueEntry = 0;
  linkSet->fullQueueEntry = -1;
}

static setIndex_t olsrLinkSetMalloc(olsrLinkSet_t *linkSet)
{
  // xSemaphoreTake(olsrLinkEmptySetLock, portMAX_DELAY);
  if(linkSet->freeQueueEntry ==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      // xSemaphoreGive(olsrLinkEmptySetLock);
      return -1;
    }
  else
    { 
      setIndex_t candidate = linkSet->freeQueueEntry;
      linkSet->freeQueueEntry = linkSet->setData[candidate].next;
      // xSemaphoreGive(olsrLinkEmptySetLock);
      //insert tlinkSet->
      // xSemaphoreTake(olsrLinkFullSetLock, portMAX_DELAY);
      setIndex_t tmp = linkSet->fullQueueEntry;
      linkSet->fullQueueEntry = candidate;
      linkSet->setData[candidate].next = tmp;
      // xSemaphoreGive(olsrLinkFullSetLock);
      return candidate;
    }
}

static bool olsrLinkSetFree(olsrLinkSet_t *linkSet,setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  // xSemaphoreTake(olsrLinkFullSetLock, portMAX_DELAY);
  setIndex_t pre = linkSet->fullQueueEntry;
  if(delItem == pre)
    {
      linkSet->fullQueueEntry =linkSet->setData[pre].next;
      // xSemaphoreGive(olsrLinkFullSetLock);
      //insert to empty queue
      // xSemaphoreTake(olsrLinkEmptySetLock,portMAX_DELAY);
      linkSet->setData[delItem].next = linkSet->freeQueueEntry;
      linkSet->freeQueueEntry = delItem;
      // xSemaphoreGive(olsrLinkEmptySetLock);
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(linkSet->setData[pre].next==delItem)
            {
              linkSet->setData[pre].next = linkSet->setData[delItem].next;
              // xSemaphoreGive(olsrLinkFullSetLock);
              //insert to empty queue
              // xSemaphoreTake(olsrLinkEmptySetLock,portMAX_DELAY);
              linkSet->setData[delItem].next = linkSet->freeQueueEntry;
              linkSet->freeQueueEntry = delItem;
              // xSemaphoreGive(olsrLinkEmptySetLock);
              return true;
            }
          pre = linkSet->setData[pre].next;
        }
    }
    return false;
}

setIndex_t olsrFindInLinkByAddr(olsrLinkSet_t *linkSet,const olsrAddr_t addr)
{
  setIndex_t it = linkSet->fullQueueEntry;
  while(it!=-1)
    {
      if(linkSet->setData[it].data.m_neighborAddr==addr)
        {
          break;
        }
      it = linkSet->setData[it].next;
    }
  return it;
}
setIndex_t olsrInsertToLinkSet(olsrLinkSet_t *linkSet,const olsrLinkTuple_t *item)
{
  setIndex_t candidate = olsrLinkSetMalloc(linkSet);
  if(candidate!=-1)
    {
      memcpy(&linkSet->setData[candidate].data,item,sizeof(olsrLinkTuple_t));
    }
  else
    {
      DEBUG_PRINT_OLSR_LINK("bad alloc whan insert by function [olsrInsertToLinkSet]\n");
    }
  return candidate;
}

void olsrPrintLinkSet(olsrLinkSet_t *linkSet)
{
  setIndex_t it = linkSet->fullQueueEntry;
  while(it != -1)
    {
      olsrLinkSetItem_t tmp = linkSet->setData[it];
      DEBUG_PRINT_OLSR_LINK("linkSet: localAddr is %d, neighborAddr is %d\n",tmp.data.m_localAddr,tmp.data.m_neighborAddr);
      it = tmp.next;
    }
}

setIndex_t olsrFindSymLinkTuple(olsrLinkSet_t *linkSet,olsrAddr_t sender,olsrTime_t now)
{
  setIndex_t candidate = linkSet->fullQueueEntry;
  while(candidate != -1)
    {
      olsrLinkSetItem_t tmp = linkSet->setData[candidate];
      if(tmp.data.m_neighborAddr == sender && tmp.data.m_symTime > now)
        {
          break;
        }
    }
  return candidate;
}
/*
************************NeighborSetFunctions********************
*/


void olsrNeighborSetInit(olsrNeighborSet_t *neighborSet)
{
  int i;
  for(i=0; i < NEIGHBOR_SET_SIZE-1; i++)
    {
      neighborSet->setData[i].next = i+1;
    }
  neighborSet->setData[i].next = -1;
  neighborSet->freeQueueEntry = 0;
  neighborSet->fullQueueEntry = -1;
}

static setIndex_t olsrNeighborSetMalloc(olsrNeighborSet_t *neighborSet)
{
  // xSemaphoreTake(olsrNeighborEmptySetLock, portMAX_DELAY);
  if(neighborSet->freeQueueEntry==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      // xSemaphoreGive(olsrNeighborEmptySetLock);
      return -1;
    }
  else
    { 
      setIndex_t candidate = neighborSet->freeQueueEntry;
      neighborSet->freeQueueEntry = neighborSet->setData[candidate].next;
      // xSemaphoreGive(olsrNeighborEmptySetLock);
      //insert to full queue
      // xSemaphoreTake(olsrNeighborFullSetLock, portMAX_DELAY);
      setIndex_t tmp = neighborSet->fullQueueEntry;
      neighborSet->fullQueueEntry = candidate;
      neighborSet->setData[candidate].next = tmp;
      // xSemaphoreGive(olsrNeighborFullSetLock);
      return candidate;
    }
}

static bool olsrNeighborSetFree(olsrNeighborSet_t *neighborSet, \
                                setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  // xSemaphoreTake(olsrNeighborFullSetLock, portMAX_DELAY);
  setIndex_t pre = neighborSet->fullQueueEntry;
  if(delItem == pre)
    {
      neighborSet->fullQueueEntry = neighborSet->setData[pre].next;
      // xSemaphoreGive(olsrNeighborFullSetLock);
      //insert to empty queue
      // xSemaphoreTake(olsrNeighborEmptySetLock,portMAX_DELAY);
      neighborSet->setData[delItem].next = neighborSet->freeQueueEntry;
      neighborSet->freeQueueEntry = delItem;
      // xSemaphoreGive(olsrNeighborEmptySetLock);
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(neighborSet->setData[pre].next==delItem)
            {
              neighborSet->setData[pre].next = neighborSet->setData[delItem].next;
              // xSemaphoreGive(olsrNeighborFullSetLock);
              //insert to empty queue
              // xSemaphoreTake(olsrNeighborEmptySetLock,portMAX_DELAY);
              neighborSet->setData[delItem].next = neighborSet->freeQueueEntry;
              neighborSet->freeQueueEntry = delItem;
              // xSemaphoreGive(olsrNeighborEmptySetLock);
              return true;
            }
          pre = neighborSet->setData[pre].next;
        }
    }
    return false;
}

setIndex_t olsrFindNeighborByAddr(olsrNeighborSet_t *neighborSet,\
                                  olsrAddr_t addr)
{
  setIndex_t it = neighborSet->fullQueueEntry;
  while(it!=-1)
    {
      if(neighborSet->setData[it].data.m_neighborAddr==addr)
        {
          break;
        }
      it = neighborSet->setData[it].next;
    }
  return it;
}

setIndex_t olsrInsertToNeighborSet(olsrNeighborSet_t *neighborSet, const olsrNeighborTuple_t* tuple)
{
  setIndex_t candidate = olsrNeighborSetMalloc(neighborSet);
  if(candidate != -1)
    {
      memcpy(&neighborSet->setData[candidate].data,tuple,sizeof(olsrNeighborTuple_t));
    }
  else
    {
      DEBUG_PRINT_OLSR_NEIGHBOR("bad alloc whan insert by function [olsrInsertToNeighborSet]\n");
    }
  return candidate;
}

void olsrPrintNeighborSet(olsrNeighborSet_t *neighborSet)
{
  setIndex_t it = neighborSet->fullQueueEntry;
  while(it != -1)
    {
      olsrNeighborSetItem_t tmp = neighborSet->setData[it];
      DEBUG_PRINT_OLSR_NEIGHBOR("neighborSet: neighborAddr is %d, status is %d(not sym is 0, sym is 1)\n",tmp.data.m_neighborAddr,tmp.data.m_status);
      it = tmp.next;
    }
}
/*
************************TwoNeighborSetFunctions********************
*/


void olsrTwoHopNeighborSetInit(olsrTwoHopNeighborSet_t *twoHopNeighborSet)
{
  int i;
  for(i=0; i < TWO_HOP_NEIGHBOR_SET_SIZE-1; i++)
    {
      twoHopNeighborSet->setData[i].next = i+1;
    }
  twoHopNeighborSet->setData[i].next = -1;
  twoHopNeighborSet->freeQueueEntry = 0;
  twoHopNeighborSet->fullQueueEntry = -1;
}

static setIndex_t olsrTwoHopNeighborSetMalloc(olsrTwoHopNeighborSet_t *twoHopNeighborSet)
{
  // xSemaphoreTake(olsrNeighborEmptySetLock, portMAX_DELAY);
  if(twoHopNeighborSet->freeQueueEntry==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      // xSemaphoreGive(olsrNeighborEmptySetLock);
      return -1;
    }
  else
    { 
      setIndex_t candidate = twoHopNeighborSet->freeQueueEntry;
      twoHopNeighborSet->freeQueueEntry = twoHopNeighborSet->setData[candidate].next;
      // xSemaphoreGive(olsrNeighborEmptySetLock);
      //insert to full queue
      // xSemaphoreTake(olsrNeighborFullSetLock, portMAX_DELAY);
      setIndex_t tmp = twoHopNeighborSet->fullQueueEntry;
      twoHopNeighborSet->fullQueueEntry = candidate;
      twoHopNeighborSet->setData[candidate].next = tmp;
      // xSemaphoreGive(olsrNeighborFullSetLock);
      return candidate;
    }
}

static bool olsrTwoHopNeighborSetFree(olsrTwoHopNeighborSet_t *twoHopNeighborSet,\
                                      setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  // xSemaphoreTake(olsrNeighborFullSetLock, portMAX_DELAY);
  setIndex_t pre = twoHopNeighborSet->fullQueueEntry;
  if(delItem == pre)
    {
      twoHopNeighborSet->fullQueueEntry = twoHopNeighborSet->setData[pre].next;
      // xSemaphoreGive(olsrNeighborFullSetLock);
      //insert to empty queue
      // xSemaphoreTake(olsrNeighborEmptySetLock,portMAX_DELAY);
      twoHopNeighborSet->setData[delItem].next = twoHopNeighborSet->freeQueueEntry;
      twoHopNeighborSet->freeQueueEntry = delItem;
      // xSemaphoreGive(olsrNeighborEmptySetLock);
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(twoHopNeighborSet->setData[pre].next==delItem)
            {
              twoHopNeighborSet->setData[pre].next = twoHopNeighborSet->setData[delItem].next;
              // xSemaphoreGive(olsrNeighborFullSetLock);
              //insert to empty queue
              // xSemaphoreTake(olsrNeighborEmptySetLock,portMAX_DELAY);
              twoHopNeighborSet->setData[delItem].next = twoHopNeighborSet->freeQueueEntry;
              twoHopNeighborSet->freeQueueEntry = delItem;
              // xSemaphoreGive(olsrNeighborEmptySetLock);
              return true;
            }
          pre = twoHopNeighborSet->setData[pre].next;
        }
    }
    return false;
}
setIndex_t olsrFindTwoHopNeighborTuple(olsrTwoHopNeighborSet_t *twoHopNeighborSet,\
                                       olsrAddr_t neighborAddr,\
                                       olsrAddr_t twoHopNeighborAddr)
{
  setIndex_t candidate = twoHopNeighborSet->fullQueueEntry;
  while(candidate != -1)
    {
      if(twoHopNeighborSet->setData[candidate].data.m_neighborAddr == neighborAddr\
      &&twoHopNeighborSet->setData[candidate].data.m_twoHopNeighborAddr == twoHopNeighborAddr)
        {
          break;
        }
    }
  return candidate;
}

setIndex_t olsrInsertToTwoHopNeighborSet(olsrTwoHopNeighborSet_t *twoHopNeighborSet,\
                                         const olsrTwoHopNeighborTuple_t* tuple)
{
  setIndex_t candidate = olsrTwoHopNeighborSetMalloc(twoHopNeighborSet);
  if(candidate != -1)
    {
      memcpy(&twoHopNeighborSet->setData[candidate].data,tuple,sizeof(olsrTwoHopNeighborTuple_t));
    }
  else
    {
      DEBUG_PRINT_OLSR_NEIGHBOR2("bad alloc.\n");
    }
  return candidate;
}

setIndex_t olsrEraseTwoHopNeighborTuple(olsrTwoHopNeighborSet_t *twoHopNeighborSet,\
                                  olsrAddr_t neighborAddr, \
                                  olsrAddr_t twoHopNeighborAddr)
{
  setIndex_t candidate = olsrFindTwoHopNeighborTuple(twoHopNeighborSet,neighborAddr,twoHopNeighborAddr);
  setIndex_t nextIt = twoHopNeighborSet->setData[candidate].next;
  if(candidate != -1)
    {
      if(olsrTwoHopNeighborSetFree(twoHopNeighborSet,candidate) != false)
        {
          return nextIt;
        }
    }
  return candidate;
}

setIndex_t olsrEraseTwoHopNeighborTupleByTuple(olsrTwoHopNeighborSet_t *twoHopNeighborSet,\
                                        olsrTwoHopNeighborTuple_t *tuple)
{
  setIndex_t candidate = olsrFindTwoHopNeighborTuple(twoHopNeighborSet,tuple->m_neighborAddr,tuple->m_twoHopNeighborAddr);
  setIndex_t nextIt = twoHopNeighborSet->setData[candidate].next;
  if(candidate != -1)
    {
      if(olsrTwoHopNeighborSetFree(twoHopNeighborSet,candidate) != false)
        {
          return nextIt;
        }
    }
  return candidate; 
}
/*
************************MprSetFunctions********************
*/

void olsrMprSetInit(olsrMprSet_t *mprSet)
{
  int i;
  for(i=0; i < MPR_SET_SIZE-1; i++)
    {
      mprSet->setData[i].next = i+1;
    }
  mprSet->setData[i].next = -1;
  mprSet->freeQueueEntry = 0;
  mprSet->fullQueueEntry = -1;
}

static setIndex_t olsrMprSetMalloc(olsrMprSet_t *mprSet)
{
  if(mprSet->freeQueueEntry==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      return -1;
    }
  else
    { 
      setIndex_t candidate = mprSet->freeQueueEntry;
      mprSet->freeQueueEntry = mprSet->setData[candidate].next;
      //insert to full queue
      setIndex_t tmp = mprSet->fullQueueEntry;
      mprSet->fullQueueEntry = candidate;
      mprSet->setData[candidate].next = tmp;
      return candidate;
    }
}

static bool olsrMprSetFree(olsrMprSet_t *mprSet, setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  setIndex_t pre = mprSet->fullQueueEntry;
  if(delItem == pre)
    {
      mprSet->fullQueueEntry = mprSet->setData[pre].next;
      //insert to empty queue
      mprSet->setData[delItem].next = mprSet->freeQueueEntry;
      mprSet->freeQueueEntry = delItem;
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(mprSet->setData[pre].next==delItem)
            {
              mprSet->setData[pre].next = mprSet->setData[delItem].next;
              //insert to empty queue
              mprSet->setData[delItem].next = mprSet->freeQueueEntry;
              mprSet->freeQueueEntry = delItem;
              return true;
            }
          pre = mprSet->setData[pre].next;
        }
    }
    return false;
}
setIndex_t olsrInsertToMprSet(olsrMprSet_t *MprSet,const olsrMprTuple_t *item)
{
  setIndex_t candidate = olsrMprSetMalloc(MprSet);
  if(candidate!=-1)
    {
      memcpy(&MprSet->setData[candidate].data,item,sizeof(olsrMprTuple_t));
    }
  else
    {
      DEBUG_PRINT_OLSR_LINK("bad alloc whan insert by function [olsrInsertToMprSet]\n");
    }
  return candidate;
}

bool olsrFindMprByAddr(olsrMprSet_t *mprSet,\
                       olsrAddr_t addr)
{
  bool isFound = false;
  setIndex_t it = mprSet->fullQueueEntry;
  while(it!=-1)
    {
      if(mprSet->setData[it].data.m_addr==addr)
        {
          isFound = true;
          break;
        }
      it = mprSet->setData[it].next;
    }
  return isFound;
}

/*
***********************MprSelectorSetFunctions********************
*/
void olsrMprSelectorSetInit(olsrMprSelectorSet_t *mprSelectorSet)
{
  int i;
  for(i=0; i < MPR_SELECTOR_SET_SIZE-1; i++)
    {
      mprSelectorSet->setData[i].next = i+1;
    }
  mprSelectorSet->setData[i].next = -1;
  mprSelectorSet->freeQueueEntry = 0;
  mprSelectorSet->fullQueueEntry = -1;
}

static setIndex_t olsrMprSelectorSetMalloc(olsrMprSelectorSet_t *mprSelectorSet)
{
  if(mprSelectorSet->freeQueueEntry==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      return -1;
    }
  else
    { 
      setIndex_t candidate = mprSelectorSet->freeQueueEntry;
      mprSelectorSet->freeQueueEntry = mprSelectorSet->setData[candidate].next;
      //insert to full queue
      setIndex_t tmp = mprSelectorSet->fullQueueEntry;
      mprSelectorSet->fullQueueEntry = candidate;
      mprSelectorSet->setData[candidate].next = tmp;
      return candidate;
    }
}

static bool olsrMprSelectorSetFree(olsrMprSelectorSet_t *mprSelectorSet, setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  setIndex_t pre = mprSelectorSet->fullQueueEntry;
  if(delItem == pre)
    {
      mprSelectorSet->fullQueueEntry = mprSelectorSet->setData[pre].next;
      //insert to empty queue
      mprSelectorSet->setData[delItem].next = mprSelectorSet->freeQueueEntry;
      mprSelectorSet->freeQueueEntry = delItem;
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(mprSelectorSet->setData[pre].next==delItem)
            {
              mprSelectorSet->setData[pre].next = mprSelectorSet->setData[delItem].next;
              //insert to empty queue
              mprSelectorSet->setData[delItem].next = mprSelectorSet->freeQueueEntry;
              mprSelectorSet->freeQueueEntry = delItem;
              return true;
            }
          pre = mprSelectorSet->setData[pre].next;
        }
    }
    return false;
}

setIndex_t olsrInsertToMprSelectorSet(olsrMprSelectorSet_t *mprSelectorSet,const olsrMprSelectorTuple_t *item)
{
  setIndex_t candidate = olsrMprSelectorSetMalloc(mprSelectorSet);
  if(candidate!=-1)
    {
      memcpy(&mprSelectorSet->setData[candidate].data,item,sizeof(olsrMprSelectorTuple_t));
    }
  else
    {
      DEBUG_PRINT_OLSR_LINK("bad alloc whan insert by function [olsrInsertToMprSelectorSet]\n");
    }
  return candidate;
}

setIndex_t olsrFindInMprSelectorSet(olsrMprSelectorSet_t *mprSelectorSet, olsrAddr_t addr)
{
  setIndex_t it = mprSelectorSet->fullQueueEntry;
  while(it != -1)
    {
      if(mprSelectorSet->setData[it].data.m_addr == addr) 
        {
          break;
        }
      it = mprSelectorSet->setData[it].next;
    }
  return it;
}

bool olsrMprSelectorSetIsEmpty()
{
  return (olsrMprSelectorSet.fullQueueEntry != -1);
}
/*
************************DuplicateSetFunctions********************
*/


void olsrDuplicateSetInit(olsrDuplicateSet_t *duplicateSet)
{
  int i;
  for(i=0; i < DUPLICATE_SET_SIZE-1; i++)
    {
      duplicateSet->setData[i].next = i+1;
    }
  duplicateSet->setData[i].next = -1;
  duplicateSet->freeQueueEntry = 0;
  duplicateSet->fullQueueEntry = -1;
}

static setIndex_t olsrDuplicateSetMalloc(olsrDuplicateSet_t *duplicateSet)
{
  if(duplicateSet->freeQueueEntry==-1)
    {
      DEBUG_PRINT_OLSR_SET("Full of sets!!!! can not malloc!!!\n");
      return -1;
    }
  else
    { 
      setIndex_t candidate = duplicateSet->freeQueueEntry;
      duplicateSet->freeQueueEntry = olsrDuplicateSet.setData[candidate].next;
      //insert to full queue
      setIndex_t tmp = duplicateSet->fullQueueEntry;
      duplicateSet->fullQueueEntry = candidate;
      duplicateSet->setData[candidate].next = tmp;
      return candidate;
    }
}

static bool olsrDuplicateSetFree(olsrDuplicateSet_t *duplicateSet, setIndex_t delItem)
{
  if(-1==delItem) return true;
  //del from full queue
  setIndex_t pre = duplicateSet->fullQueueEntry;
  if(delItem == pre)
    {
      duplicateSet->fullQueueEntry = duplicateSet->setData[pre].next;
      //insert to empty queue
      duplicateSet->setData[delItem].next = duplicateSet->freeQueueEntry;
      duplicateSet->freeQueueEntry = delItem;
      return true;
    }
  else 
    {
      while(pre!=-1)
        {
          if(duplicateSet->setData[pre].next==delItem)
            {
              duplicateSet->setData[pre].next = duplicateSet->setData[delItem].next;
              //insert to empty queue
              duplicateSet->setData[delItem].next = duplicateSet->freeQueueEntry;
              duplicateSet->freeQueueEntry = delItem;
              return true;
            }
          pre = duplicateSet->setData[pre].next;
        }
    }
    return false;
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
  olsrTwoHopNeighborEmptySetLock = xSemaphoreCreateMutex();
  olsrTwoHopNeighborFullSetLock = xSemaphoreCreateMutex();
}


void olsrStructInitAll(dwDevice_t *dev)
{
  olsrSendQueueInit();
  olsrRecvQueueInit();
  olsrSetEntryInit();
  olsrTopologySetInit(&olsrTopologySet);
  olsrLinkSetInit(&olsrLinkSet);
  olsrNeighborSetInit(&olsrNeighborSet);
  olsrTwoHopNeighborSetInit(&olsrTwoHopNeighborSet);
  olsrMprSetInit(&olsrMprSet);
  olsrDuplicateSetInit(&olsrDuplicateSet);
}
