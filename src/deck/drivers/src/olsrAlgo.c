#include "libdw1000.h"
#include "FreeRTOS.h"
#include <string.h>
#include <semphr.h>
#include "task.h"
#include "locodeck.h"
#include "olsrDebug.h"
#include "mac.h"
#include "olsrAlgo.h"
#include "olsrStruct.h"
#include "olsrPacket.h"

//const

#define OLSR_HELLO_INTERVAL 2000
#define OLSR_NEIGHB_HOLD_TIME (3*OLSR_HELLO_INTERVAL)
#define OLSR_TC_INTERVAL 5000
#define OLSR_TOP_HOLD_TIME (3*OLSR_TC_INTERVAL)
#define TS_INTERVAL 1000

/// Unspecified link type.
#define OLSR_UNSPEC_LINK        0
/// Asymmetric link type.
#define OLSR_ASYM_LINK          1
/// Symmetric link type.
#define OLSR_SYM_LINK           2
/// Lost link type.
#define OLSR_LOST_LINK          3

/// Not neighbor type.
#define OLSR_NOT_NEIGH          0
/// Symmetric neighbor type.
#define OLSR_SYM_NEIGH          1
/// Asymmetric neighbor type.
#define OLSR_MPR_NEIGH          2



static dwDevice_t* dwm;
extern uint16_t myAddress;
extern xQueueHandle g_olsrSendQueue;
extern xQueueHandle g_olsrRecvQueue;
static uint16_t g_staticMessageSeq = 0;
static uint16_t g_ansn = 0;
static SemaphoreHandle_t olsrMessageSeqLock;
static SemaphoreHandle_t olsrAnsnLock;
packet_t rxPacket;
//TODO define packet and message struct once, save space
//debugging, to be deleted
//TODO delete testDataLength2send


//rxcallback
void olsrDeviceInit(dwDevice_t *dev){
  dwm = dev;
  olsrMessageSeqLock = xSemaphoreCreateMutex();
  olsrAnsnLock = xSemaphoreCreateMutex();
}
static uint16_t getSeqNumber()
{
  uint16_t retVal= 0;
  xSemaphoreTake(olsrMessageSeqLock,portMAX_DELAY);
  retVal = g_staticMessageSeq++;
  xSemaphoreGive(olsrMessageSeqLock);
  return retVal;
}
void olsrRxCallback(dwDevice_t *dev){
    DEBUG_PRINT_OLSR_SYSTEM("rxCallBack\n");
    int dataLength = dwGetDataLength(dwm);
    if(dataLength==0){
        DEBUG_PRINT_OLSR_RECEIVE("DataLen=0!\n");
				return;
		}
    memset(&rxPacket,0,sizeof(packet_t));
    dwGetData(dwm,(uint8_t*)&rxPacket,dataLength);
		
    DEBUG_PRINT_OLSR_RECEIVE("dataLength=%d\n", dataLength);
    DEBUG_PRINT_OLSR_RECEIVE("fcf: %X\n", rxPacket.fcf);
    if(rxPacket.fcf_s.type!=MAC802154_TYPE_OLSR){
        DEBUG_PRINT_OLSR_RECEIVE("Mac_T not OLSR!\n");
				return;
		}
    #ifdef DEBUG_OLSR_RECEIVE_DECODEHDR
		locoAddress_t u64 = 0;
    DEBUG_PRINT_OLSR_RECEIVE("dataLength=%d\n", dataLength);
    DEBUG_PRINT_OLSR_RECEIVE("fcf: %X\n", rxPacket.fcf);
    DEBUG_PRINT_OLSR_RECEIVE("seq: %X\n", rxPacket.seq);
    DEBUG_PRINT_OLSR_RECEIVE("pan: %X\n", rxPacket.pan);
    u64=rxPacket.destAddress;
    DEBUG_PRINT_OLSR_RECEIVE("dst: %X%X%X\n", u64>>32, u64, u64);
    u64=rxPacket.sourceAddress;
    DEBUG_PRINT_OLSR_RECEIVE("src: %X%X%X\n", u64>>32, u64, u64);
    #endif
    xQueueSend(g_olsrRecvQueue,&rxPacket,portMAX_DELAY);
}
//routing table compute
void olsr_routing_table_compute(){
    DEBUG_PRINT_OLSR_SYSTEM("ROUTING TABLE COMPUTE!\n");
}
//packet process

void olsr_tc_forward(olsrMessage_t* tc_message){

}
void olsr_ts_process(const olsrMessage_t* ts_msg){

}

static void incrementAnsn()
{
  xSemaphoreTake(olsrAnsnLock,portMAX_DELAY);
  g_ansn++;
  xSemaphoreGive(olsrAnsnLock);
}
static uint16_t getAnsn()
{
  xSemaphoreTake(olsrAnsnLock,portMAX_DELAY); 
  uint16_t retVal = g_ansn;
  xSemaphoreGive(olsrAnsnLock);
  return retVal;
}
static void addNeighborTuple(const olsrNeighborTuple_t* tuple)
{
  olsrInsertToNeighborSet(&olsrNeighborSet,tuple);
  incrementAnsn();
}

static void addTwoHopNeighborTuple(const olsrTwoHopNeighborTuple_t* tuple)
{
  olsrInsertToTwoHopNeighborSet(&olsrTwoHopNeighborSet,tuple);
}

static void addMprSelectorTuple(const olsrMprSelectorTuple_t * tuple)
{
  olsrInsertToMprSelectorSet(&olsrMprSelectorSet,tuple);
  incrementAnsn();
}

static void addTopologyTuple(const olsrTopologyTuple_t *tuple)
{
  olsrInsertToTopologySet(&olsrTopologySet,tuple);
}

static void linkTupleAdded(olsrLinkTuple_t *tuple,uint8_t willingness)
{
  // Creates associated neighbor tuple
  olsrNeighborTuple_t nbTuple;
  nbTuple.m_neighborAddr = tuple->m_neighborAddr;
  nbTuple.m_willingness = willingness;
  olsrTime_t now = xTaskGetTickCount();
  if(tuple->m_symTime >= now)
    {
      nbTuple.m_status = STATUS_SYM;
    }
  else
    {
      nbTuple.m_status = STATUS_NOT_SYM;
    }
  addNeighborTuple(&nbTuple);
}
static void linkTupleUpdated(olsrLinkTuple_t *tuple,uint8_t willingness)
{
  //// Each time a link tuple changes, the associated neighbor tuple must be recomputed
  setIndex_t neighborTuple = olsrFindNeighborByAddr(&olsrNeighborSet,\
                                                    tuple->m_neighborAddr);
  if(neighborTuple==-1)
    {
      linkTupleAdded(tuple,willingness);
      neighborTuple = olsrFindNeighborByAddr(&olsrNeighborSet,\
                                            tuple->m_neighborAddr);
    }
  
  if(neighborTuple!=-1)
    {
      bool hasSymmetricLink = false;
      setIndex_t linkTupleIter = olsrLinkSet.fullQueueEntry;
      while(linkTupleIter!=-1)
        {
          if(olsrLinkSet.setData[linkTupleIter].data.m_neighborAddr == tuple->m_neighborAddr &&\
          olsrLinkSet.setData[linkTupleIter].data.m_symTime >= xTaskGetTickCount())
            {
              hasSymmetricLink = true;
              break;
            }
          linkTupleIter = olsrLinkSet.setData[linkTupleIter].next;
        }
      if(hasSymmetricLink)
        {
          olsrNeighborSet.setData[neighborTuple].data.m_status = STATUS_SYM;
        }
      else
        {
          olsrNeighborSet.setData[neighborTuple].data.m_status = STATUS_NOT_SYM;
        }
      
    }
  else
    {
      DEBUG_PRINT_OLSR_LINK("bad alloc in linkTupleUpdated func!\n");
    }
  

}
static void linkSensing(const olsrMessage_t* helloMsg)
{
  configASSERT(helloMsg->m_messageHeader.m_vTime>0);
  xSemaphoreTake(olsrLinkFullSetLock,portMAX_DELAY); //Seamphore Take
  setIndex_t linkTuple = olsrFindInLinkByAddr(&olsrLinkSet, helloMsg->m_messageHeader.m_originatorAddress);
  olsrTime_t now = xTaskGetTickCount();
  bool created = false, updated = false;
  if(linkTuple == -1)
    {
      DEBUG_PRINT_OLSR_LINK("not found Addr %d in linkSet",helloMsg->m_messageHeader.m_originatorAddress);
      olsrLinkTuple_t newLinkTuple;
      newLinkTuple.m_localAddr = myAddress;
      newLinkTuple.m_neighborAddr = helloMsg->m_messageHeader.m_originatorAddress;
      newLinkTuple.m_symTime = now - M2T(1000);
      newLinkTuple.m_expirationTime = now + helloMsg->m_messageHeader.m_vTime*1000;
      linkTuple = olsrInsertToLinkSet(&olsrLinkSet,&newLinkTuple);
      if(linkTuple==-1)
        {
          DEBUG_PRINT_OLSR_LINK("can not malloc from link set by Function[linkSensing]\n");
          return;
        }
      created = true;
    }
  else
    {
      updated = true;
    }
  olsrLinkSet.setData[linkTuple].data.m_asymTime = now +helloMsg->m_messageHeader.m_vTime;
  olsrHelloMessage_t* helloMsgBody = (olsrHelloMessage_t*)(helloMsg->m_messagePayload);
  for(uint8_t i = 0;i < helloMsgBody->m_helloHeader.m_linkMessageNumber;i++)
    {
      uint8_t lt = helloMsgBody->m_linkMessage[i].m_linkCode & 0x03;
      uint8_t nt = (helloMsgBody->m_linkMessage[i].m_linkCode >> 2 ) & 0x03;

      if((lt == OLSR_SYM_LINK && nt == OLSR_NOT_NEIGH)\
      ||(nt != OLSR_SYM_NEIGH && nt != OLSR_MPR_NEIGH\
      && nt != OLSR_NOT_NEIGH))
        {
          continue;
        }
      if(helloMsgBody->m_linkMessage[i].m_addresses == myAddress)
        {
          if(lt == OLSR_LOST_LINK)
            {
              olsrLinkSet.setData[linkTuple].data.m_symTime = now - M2T(1000);
              updated = true;
            }
          else if(lt == OLSR_SYM_LINK ||lt == OLSR_ASYM_LINK)
            {
              olsrLinkSet.setData[linkTuple].data.m_symTime = now +helloMsg->m_messageHeader.m_vTime;
              olsrLinkSet.setData[linkTuple].data.m_expirationTime = olsrLinkSet.setData[linkTuple].data.m_symTime+OLSR_NEIGHB_HOLD_TIME*1000;
              updated = true;
            }
          else
            {
              DEBUG_PRINT_OLSR_LINK("BAD LINK");
            }  
        }
      else
        {
          DEBUG_PRINT_OLSR_LINK("this %d is not equal to myaddress\n",helloMsgBody->m_linkMessage[i].m_addresses);
        }      
    }
  olsrLinkSet.setData[linkTuple].data.m_expirationTime = olsrLinkSet.setData[linkTuple].data.m_asymTime > \
                                                olsrLinkSet.setData[linkTuple].data.m_expirationTime? \
                                                olsrLinkSet.setData[linkTuple].data.m_asymTime:\
                                                olsrLinkSet.setData[linkTuple].data.m_expirationTime;
  xSemaphoreTake(olsrNeighborFullSetLock,portMAX_DELAY);
  if(updated)
    {
      linkTupleUpdated(&olsrLinkSet.setData[linkTuple].data,helloMsgBody->m_helloHeader.m_willingness);
    }
  if(created)
    {
      linkTupleAdded(&olsrLinkSet.setData[linkTuple].data,helloMsgBody->m_helloHeader.m_willingness); // same addr?
      //del
    }
  xSemaphoreGive(olsrNeighborFullSetLock); 
  xSemaphoreGive(olsrLinkFullSetLock); //Seamphore Give
}

void populateNeighborSet(const olsrMessage_t* helloMsg)
{
  xSemaphoreTake(olsrNeighborFullSetLock,portMAX_DELAY);
  setIndex_t nbTuple = olsrFindNeighborByAddr(&olsrNeighborSet,
                                              helloMsg->m_messageHeader.m_originatorAddress);
  if(nbTuple != -1)
    {
      olsrNeighborSet.setData[nbTuple].data.m_willingness = ((olsrHelloMessageHeader_t *)helloMsg->m_messagePayload)->m_willingness;
    }
  xSemaphoreGive(olsrNeighborFullSetLock); 
}

void populateTwoHopNeighborSet(const olsrMessage_t* helloMsg)
{
  xSemaphoreTake(olsrLinkFullSetLock,portMAX_DELAY);
  xSemaphoreTake(olsrTwoHopNeighborFullSetLock,portMAX_DELAY);
  olsrTime_t now = xTaskGetTickCount();
  olsrAddr_t sender = helloMsg->m_messageHeader.m_originatorAddress;
  setIndex_t linkTuple = olsrFindInLinkByAddr(&olsrLinkSet, sender);
  if(linkTuple != -1)
    {
      olsrHelloMessage_t* helloMsgBody = (olsrHelloMessage_t*)helloMsg->m_messagePayload;
      for(int i = 0;i<helloMsgBody->m_helloHeader.m_linkMessageNumber;i++)
        {
          uint8_t nbType = (helloMsgBody->m_linkMessage[i].m_linkCode >> 2) & 0x3;
          olsrAddr_t candidate = helloMsgBody->m_linkMessage[i].m_addresses;
          if(nbType == OLSR_SYM_NEIGH || nbType == OLSR_MPR_NEIGH)
            {
              if(candidate == myAddress)
                {
                  continue;
                }
              setIndex_t twoHopNeighborTuple = olsrFindTwoHopNeighborTuple(&olsrTwoHopNeighborSet,sender,candidate);
              if(twoHopNeighborTuple != -1)
                {
                  olsrTwoHopNeighborSet.setData[twoHopNeighborTuple].data.m_expirationTime = now+\
                                                    helloMsg->m_messageHeader.m_vTime;
                }
              else
                {
                  olsrTwoHopNeighborTuple_t newTuple;
                  newTuple.m_neighborAddr = sender;
                  newTuple.m_twoHopNeighborAddr = candidate;
                  newTuple.m_expirationTime = now+helloMsg->m_messageHeader.m_vTime;
                  addTwoHopNeighborTuple(&newTuple);
                }
              
            }
          else if(nbType == OLSR_NOT_NEIGH)
            {
              olsrEraseTwoHopNeighborTuple(&olsrTwoHopNeighborSet, sender,candidate);
            }
          else
            {
              DEBUG_PRINT_OLSR_NEIGHBOR2("bad neighbor type in func [PopulateTwoHopNeighborSet]\n");
            }
          
        }
    }
  else
    {
      DEBUG_PRINT_OLSR_NEIGHBOR2("can not found link tuple\n");
    }
  xSemaphoreGive(olsrTwoHopNeighborFullSetLock);
  xSemaphoreGive(olsrLinkFullSetLock);
}

void populateMprSelectorSet(const olsrMessage_t* helloMsg)
{
  olsrTime_t now = xTaskGetTickCount();
  olsrAddr_t sender =  helloMsg->m_messageHeader.m_originatorAddress;
  olsrHelloMessage_t* helloBody = (olsrHelloMessage_t *)helloMsg->m_messagePayload;
  for(int i = 0;i< helloBody->m_helloHeader.m_linkMessageNumber;i++)
    {
      uint8_t nt = helloBody->m_linkMessage[i].m_linkCode >> 2;
      if(nt == OLSR_MPR_NEIGH && helloBody->m_linkMessage[i].m_addresses == myAddress)
        {
          setIndex_t candidate = olsrFindInMprSelectorSet(&olsrMprSelectorSet,sender);
          if(candidate == -1)
            {
              olsrMprSelectorTuple_t new;
              new.m_addr = sender;
              new.m_expirationTime = now + helloMsg->m_messageHeader.m_vTime;
              addMprSelectorTuple(&new);
            }
          else
            {
              olsrMprSelectorSet.setData[candidate].data.m_expirationTime = now + helloMsg->m_messageHeader.m_vTime;
            }
        }
    }

}
void coverTwoHopNeighbors(olsrAddr_t addr, olsrTwoHopNeighborSet_t *twoHopNeighborSet)
{
  setIndex_t n2NeedToEraseIt = twoHopNeighborSet->fullQueueEntry;
  while(n2NeedToEraseIt != -1)
    {
      olsrTwoHopNeighborTuple_t item = twoHopNeighborSet->setData[n2NeedToEraseIt].data;
      if(item.m_neighborAddr == addr)
        {
          n2NeedToEraseIt = olsrEraseTwoHopNeighborTupleByTuple(twoHopNeighborSet,&item);
          continue;
        }
      n2NeedToEraseIt = twoHopNeighborSet->setData[n2NeedToEraseIt].next;
    }  
}
void mprCompute()
{
  olsrNeighborSet_t N;
  olsrNeighborSetInit(&N);

  setIndex_t itForolsrNeighborSet = olsrNeighborSet.fullQueueEntry;
  while(itForolsrNeighborSet != -1)
    {
      if(olsrNeighborSet.setData[itForolsrNeighborSet].data.m_status == STATUS_SYM)
        {
          olsrInsertToNeighborSet(&N,&olsrNeighborSet.setData[itForolsrNeighborSet].data);
        }
      itForolsrNeighborSet = olsrNeighborSet.setData[itForolsrNeighborSet].next;
    }
  
  olsrTwoHopNeighborSet_t N2;
  olsrTwoHopNeighborSetInit(&N2);

  setIndex_t itForTwoHopNeighBorSet = olsrTwoHopNeighborSet.fullQueueEntry;
  while(itForTwoHopNeighBorSet != -1)
    {
      olsrTwoHopNeighborSetItem_t twoHopNTuple = olsrTwoHopNeighborSet.setData[itForTwoHopNeighBorSet];
      //two hop neighbor can not equal to myself
      if(twoHopNTuple.data.m_twoHopNeighborAddr == myAddress)
        {
          itForTwoHopNeighBorSet = twoHopNTuple.next;
          continue;
        }
      
      bool ok = false;
      setIndex_t itForN = N.fullQueueEntry;
      while(itForN != -1)
        {
          if(N.setData[itForN].data.m_neighborAddr == twoHopNTuple.data.m_neighborAddr)
            {
              if(N.setData[itForN].data.m_willingness == WILL_NEVER)
                {
                  ok = false;
                  break;
                }
              else
                {
                  ok = true;
                  break;
                }
            }
        }
      if(!ok)
        {
          itForTwoHopNeighBorSet = twoHopNTuple.next;
          continue;
        }
      //this two hop neighbor can not be one hop neighbor
      itForN = N.fullQueueEntry;
      while(itForN != -1)
        {
          if(N.setData[itForN].data.m_neighborAddr == twoHopNTuple.data.m_twoHopNeighborAddr)
            {
              ok = false;
              break;
            }
            itForN = N.setData[itForN].next;
        }

      if(ok)
        {
          olsrInsertToTwoHopNeighborSet(&N2,&twoHopNTuple.data);
        }

      itForTwoHopNeighBorSet = twoHopNTuple.next;
    }

  olsrAddr_t coveredTwoHopNeighbors[TWO_HOP_NEIGHBOR_SET_T];
  uint8_t lenthOfCoveredTwoHopNeighbor = 0;
    //find the unique pair of two hop neighborN2
  setIndex_t n2It = N2.fullQueueEntry;
  while(n2It != -1)
    {
      bool onlyOne = true;
      setIndex_t otherN2It = N2.fullQueueEntry;
      while(otherN2It != -1)
        {
          if(N2.setData[otherN2It].data.m_twoHopNeighborAddr == N2.setData[n2It].data.m_twoHopNeighborAddr\
            &&N2.setData[otherN2It].data.m_neighborAddr != N2.setData[n2It].data.m_neighborAddr)
            {
              onlyOne = false;
              break;
            }
          otherN2It = N2.setData[otherN2It].next;
        }
      
      if(onlyOne&&!olsrFindMprByAddr(&olsrMprSet, N2.setData[n2It].data.m_neighborAddr))
        {
          olsrMprTuple_t item;
          item.m_addr = N2.setData[n2It].data.m_neighborAddr;
          olsrInsertToMprSet(&olsrMprSet,&item);
          
          setIndex_t twoHopNIt = N2.fullQueueEntry;
          while(twoHopNIt != -1)
            {
              if(N2.setData[twoHopNIt].data.m_neighborAddr == N2.setData[n2It].data.m_neighborAddr)
                {
                  coveredTwoHopNeighbors[lenthOfCoveredTwoHopNeighbor++] = N2.setData[twoHopNIt].data.m_twoHopNeighborAddr;
                }
              twoHopNIt = N2.setData[twoHopNIt].next;
            }
        }
      n2It = N2.setData[n2It].next;
    }

  //erase 
  setIndex_t itForEraseFromN2 = N2.fullQueueEntry;
  while(itForEraseFromN2 != -1)
    {
      bool find = false;
      for(int i=0;i<lenthOfCoveredTwoHopNeighbor;i++)
        {
          if(coveredTwoHopNeighbors[lenthOfCoveredTwoHopNeighbor] \
              == N2.setData[itForEraseFromN2].data.m_twoHopNeighborAddr)
            {
              find = true;
              break;
            }
        }
      if(find)
        {
          setIndex_t tmp = itForEraseFromN2;
          itForEraseFromN2 = olsrEraseTwoHopNeighborTupleByTuple(&N2,&N2.setData[itForEraseFromN2].data);
          if(tmp != itForEraseFromN2) continue;
        }
      itForEraseFromN2 = N2.setData[itForEraseFromN2].next;
    }
  while(N2.fullQueueEntry != -1)
      {
        int maxR = 0;
        setIndex_t maxNeighborTuple = -1;
        uint8_t num[NEIGHBOR_SET_SIZE];
        setIndex_t nbIt = N.fullQueueEntry;
        while(nbIt != -1)
          {
            setIndex_t nbTwoHopIt = N2.fullQueueEntry;
            uint8_t count = 0;
            while(nbTwoHopIt != -1)
              {
                if(N2.setData[nbTwoHopIt].data.m_neighborAddr \
                  == N.setData[nbIt].data.m_neighborAddr)
                {
                  count++;
                }
                nbTwoHopIt = N2.setData[nbTwoHopIt].next;
              }
            num[nbIt] = count;
            nbIt = N.setData[nbIt].next;
          }
        for(int i = 0;i<NEIGHBOR_SET_SIZE;i++)
          {
            if(num[i]>maxR)
              {
                maxR = num[i];
                maxNeighborTuple = i;
              }
          }
        if(maxNeighborTuple != -1)
          {
            olsrMprTuple_t tmp;
            tmp.m_addr = N.setData[maxNeighborTuple].data.m_neighborAddr;
            olsrInsertToMprSet(&olsrMprSet,&tmp);
            coverTwoHopNeighbors(tmp.m_addr,&N2);
          }
      }

}
void olsrPrintAll()
{
  olsrPrintLinkSet(&olsrLinkSet);
  olsrPrintNeighborSet(&olsrNeighborSet);
}
void olsrProcessHello(const olsrMessage_t* helloMsg)
{
  linkSensing(helloMsg);
  populateNeighborSet(helloMsg);
  populateTwoHopNeighborSet(helloMsg);
  mprCompute();
  populateMprSelectorSet(helloMsg);
  olsrPrintAll();
}
//
void olsrProcessTc(const olsrMessage_t* tcMsg)
{
  olsrTime_t now = xTaskGetTickCount();
  olsrAddr_t originator = tcMsg->m_messageHeader.m_originatorAddress;
  olsrAddr_t sender = tcMsg->m_messageHeader.m_relayAddress;
  olsrTopologyMessage_t* tcBody = (olsrTopologyMessage_t *)tcMsg->m_messagePayload;
  uint16_t ansn = tcBody->m_ansn;

  setIndex_t linkTupleIndex = olsrFindSymLinkTuple(&olsrLinkSet,sender,now);
  if(linkTupleIndex == -1)
    {
      DEBUG_PRINT_OLSR_TC("not from sym link.\n");
      return;
    }
  setIndex_t topologyTupleIndex = olsrFindNewerTopologyTuple(&olsrTopologySet,originator,ansn);
  if(topologyTupleIndex != -1)
    {
      return;
    }
  olsrEraseOlderTopologyTuples(&olsrTopologySet,originator,ansn);
  uint8_t count = (tcMsg->m_messageHeader.m_messageSize - sizeof(olsrMessageHeader_t) - 2)/ \
                  sizeof(olsrTopologyMessageUint_t);
  for(int i = 0;i < count ;i++)
    {
      olsrAddr_t destAddr =  tcBody->m_content[i].m_address;
      setIndex_t topologyIt = olsrFindTopologyTuple(&olsrTopologySet,destAddr,originator);
      if(topologyIt != -1)
        {
          olsrTopologySet.setData[topologyIt].data.m_expirationTime = now + tcMsg->m_messageHeader.m_vTime;
        }
      else
        {
          olsrTopologyTuple_t topologyTuple;
          topologyTuple.m_destAddr = destAddr;
          topologyTuple.m_lastAddr = originator;
          topologyTuple.m_seqenceNumber = ansn;
          topologyTuple.m_expirationTime = now + tcMsg->m_messageHeader.m_vTime;
          topologyTuple.m_distance = tcBody->m_content[i].m_distance;
          addTopologyTuple(&topologyTuple);
        }
    }
}

//switch to tc|hello|ts process
void olsrPacketDispatch(const packet_t* rxPacket)
{
  DEBUG_PRINT_OLSR_SYSTEM("PACKET_DISPATCH\n");  
  olsrPacket_t* olsrPacket = (olsrPacket_t *)rxPacket->payload;
  //need to add a condition whether recvive a packet from self
  uint16_t packetSize = olsrPacket->m_packetHeader.m_packetLength;
  uint16_t index = sizeof(olsrPacketHeader_t);
  uint8_t* olsrMessageIdx = olsrPacket->m_packetPayload;
  olsrMessage_t* olsrMessage = (olsrMessage_t*)olsrMessageIdx;
  DEBUG_PRINT_OLSR_RECEIVE("RECV PACKET LEN: %d\n", packetSize);  
  while(index<packetSize)
    {
      olsrMessageHeader_t* messageHeader = (olsrMessageHeader_t*)olsrMessage;
      olsrMessageType_t type = messageHeader->m_messageType;
				#ifdef DEBUG_OLSR_RECEIVE
        DEBUG_PRINT_OLSR_RECEIVE("HDR size\t%d\n", messageHeader->m_messageSize);
        DEBUG_PRINT_OLSR_RECEIVE("HDR seq\t%d\n", messageHeader->m_messageSeq);
				#endif //DEBUG_OLSR_RECEIVE
      switch (type) 
        {
        case HELLO_MESSAGE:
            DEBUG_PRINT_OLSR_RECEIVE("HELLO_MESSAGE\n");
            olsrProcessHello(olsrMessage);
            break;
        case TC_MESSAGE:
            DEBUG_PRINT_OLSR_RECEIVE("TC_MESSAGE\n");
            olsrProcessTc(olsrMessage);
            // olsr_tc_forward(olsr_message);
            break;
        case DATA_MESSAGE:
            DEBUG_PRINT_OLSR_RECEIVE("DATA_MESSAGE\n");
            break;
        case TS_MESSAGE:
            DEBUG_PRINT_OLSR_RECEIVE("TS_MESSAGE\n");
            //olsr_ts_process(olsr_message);
            break;
        default:
            DEBUG_PRINT_OLSR_RECEIVE("WRONG MESSAGE\n");
            break;
        }
        index += messageHeader->m_messageSize;
        olsrMessageIdx += messageHeader->m_messageSize;
        olsrMessage = (olsrMessage_t*)olsrMessageIdx;
    }
}

//send related function

void olsrSendHello()
{
  olsrMessage_t msg;
  //message header initial
  msg.m_messageHeader.m_messageType = HELLO_MESSAGE;
  msg.m_messageHeader.m_vTime = OLSR_NEIGHB_HOLD_TIME;
  msg.m_messageHeader.m_messageSize = sizeof(olsrMessageHeader_t);
  msg.m_messageHeader.m_originatorAddress = myAddress;
  msg.m_messageHeader.m_destinationAddress = 0;
  msg.m_messageHeader.m_relayAddress = myAddress;  
  msg.m_messageHeader.m_timeToLive = 0xff;
  msg.m_messageHeader.m_hopCount = 0;
  msg.m_messageHeader.m_messageSeq = getSeqNumber();
  //hello message
  olsrHelloMessage_t helloMessage;
  helloMessage.m_helloHeader.m_hTime = OLSR_HELLO_INTERVAL; //hello's header on packet
  helloMessage.m_helloHeader.m_willingness = WILL_ALWAYS;
  helloMessage.m_helloHeader.m_linkMessageNumber = 0;

  //loop
  xSemaphoreTake(olsrLinkFullSetLock,portMAX_DELAY);
  setIndex_t linkTupleIndex = olsrLinkSet.fullQueueEntry;
  olsrTime_t now = xTaskGetTickCount();
  while(linkTupleIndex!=-1)
    {
      if(!(olsrLinkSet.setData[linkTupleIndex].data.m_localAddr == myAddress &&\
      olsrLinkSet.setData[linkTupleIndex].data.m_expirationTime >= now))
        {
          linkTupleIndex = olsrLinkSet.setData[linkTupleIndex].next;
          continue;
        }
      uint8_t linkType, nbType = 0xff;

      if(olsrLinkSet.setData[linkTupleIndex].data.m_symTime>=now)
        {
          linkType = OLSR_SYM_LINK;//2
        }
      else if(olsrLinkSet.setData[linkTupleIndex].data.m_asymTime>=now)
        {
          linkType = OLSR_ASYM_LINK;//1
        }
      else
        {
          linkType = OLSR_LOST_LINK;//3
        }
      if(olsrFindMprByAddr(&olsrMprSet, olsrLinkSet.setData[linkTupleIndex].data.m_neighborAddr))
        {
           nbType = OLSR_MPR_NEIGH;//2
        }
      else
        {
          bool ok = false;
          setIndex_t neighborTupleIndex = olsrNeighborSet.fullQueueEntry;
          while(neighborTupleIndex!=-1)
            {
              if(olsrNeighborSet.setData[neighborTupleIndex].data.m_neighborAddr ==\
              olsrLinkSet.setData[linkTupleIndex].data.m_neighborAddr) // this linkTuple Addr is in NighborSet
                {
                  if(olsrNeighborSet.setData[neighborTupleIndex].data.m_status == STATUS_SYM)
                    {
                      nbType = OLSR_SYM_NEIGH; //is a sym neighbor
                    }
                  else if(olsrNeighborSet.setData[neighborTupleIndex].data.m_status == STATUS_NOT_SYM)
                    {
                      nbType = OLSR_NOT_NEIGH; // is not a sym neghbor
                    }
                  else
                    {
                      DEBUG_PRINT_OLSR_HELLO("There is a neighbor tuple with an unknown status!\n");
                    }
                  ok = true;
                  break;
                }
            }
          if(!ok)
            {
              linkTupleIndex = olsrLinkSet.setData[linkTupleIndex].next;
              continue;
            }
        }
          olsrLinkMessage_t linkMessage;
          linkMessage.m_linkCode = (linkType & 0x03) | ((nbType << 2) & 0x0f);
          linkMessage.m_addressUsedSize = 1;
          linkMessage.m_addresses = olsrLinkSet.setData[linkTupleIndex].data.m_neighborAddr;
          if(helloMessage.m_helloHeader.m_linkMessageNumber==LINK_MESSAGE_MAX_NUM) break;
          helloMessage.m_linkMessage[helloMessage.m_helloHeader.m_linkMessageNumber++] = linkMessage;
          linkTupleIndex = olsrLinkSet.setData[linkTupleIndex].next;
    }
  xSemaphoreGive(olsrLinkFullSetLock);
  uint16_t writeSize = sizeof(olsrHelloMessageHeader_t)+helloMessage.m_helloHeader.m_linkMessageNumber*\
                       sizeof(olsrLinkMessage_t);
  msg.m_messageHeader.m_messageSize +=  writeSize;                 
  memcpy(msg.m_messagePayload,&helloMessage,writeSize);
  xQueueSend(g_olsrSendQueue,&msg,portMAX_DELAY);
}


void olsrSendTc()
{
  olsrMessage_t msg;
  msg.m_messageHeader.m_messageType = TC_MESSAGE;
  msg.m_messageHeader.m_vTime = OLSR_TOP_HOLD_TIME;
  msg.m_messageHeader.m_messageSize = sizeof(olsrMessageHeader_t);
  msg.m_messageHeader.m_originatorAddress = myAddress;
  msg.m_messageHeader.m_destinationAddress = 0;
  msg.m_messageHeader.m_relayAddress = 0;  
  msg.m_messageHeader.m_timeToLive = 0xff;
  msg.m_messageHeader.m_hopCount = 0;
  msg.m_messageHeader.m_messageSeq = getSeqNumber();

  olsrTopologyMessage_t tcMsg;
  tcMsg.m_ansn = getAnsn();
  
  setIndex_t mprSelectorIt = olsrMprSelectorSet.fullQueueEntry;
  uint8_t pos = 0;
  while(mprSelectorIt != -1)
    {
      olsrMprSelectorSetItem_t tmp = olsrMprSelectorSet.setData[mprSelectorIt];
      tcMsg.m_content[pos].m_address = tmp.data.m_addr;
      tcMsg.m_content[pos++].m_distance = 1;
      mprSelectorIt = tmp.next;
    }
  memcpy(&msg.m_messagePayload,&tcMsg,2+pos*sizeof(olsrTopologyMessageUint_t));
  msg.m_messageHeader.m_messageSize+=(2+pos*sizeof(olsrTopologyMessageUint_t));
  xQueueSend(g_olsrSendQueue,&msg,portMAX_DELAY);
}

// all task defination
void olsrHelloTask(void *ptr){
    while (true)
    {
        /* code */
        DEBUG_PRINT_OLSR_SEND("HELLO_SEND TO QUEUE\n");
        olsrSendHello();
        vTaskDelay(M2T(OLSR_HELLO_INTERVAL));
    }
}
void olsrTcTask(void *ptr){
    DEBUG_PRINT_OLSR_SYSTEM("TC_SEND TO QUEUE\n");
    while(true)
    {
      if(!olsrMprSelectorSetIsEmpty())
        {
          olsrSendTc();
        }
      else
        {
          DEBUG_PRINT_OLSR_TC("Not sending any TC, no one selected me as MPR.\n");
        }     
      vTaskDelay(M2T(OLSR_TC_INTERVAL));
    }
}
void olsr_send_ts_to_queue(){
    DEBUG_PRINT_OLSR_SEND("TS_SEND TO QUEUE\n");
}

void olsr_ts_task(void *ptr){
    while(true)
    {    
        olsr_send_ts_to_queue();
        vTaskDelay(M2T(TS_INTERVAL)); 
    }
}
packet_t dwPacket = {0};
void olsrSendTask(void *ptr)
{
    //pointer initialize
    bool hasMessageCache =false;
    olsrMessage_t olsrMessageCache = {0};
  	MAC80215_PACKET_INIT(dwPacket, MAC802154_TYPE_OLSR)
    dwm = (dwDevice_t *)ptr; // can remove
    olsrPacket_t *olsrPacket = (olsrPacket_t *)dwPacket.payload;
    olsrMessage_t *messages;
    //task loop
    int count = 0;
    while(true){
      messages = (olsrMessage_t *)olsrPacket->m_packetPayload;
      uint8_t *writePosition = (uint8_t *)messages;
      if(hasMessageCache){
        configASSERT(olsrMessageCache.m_messageHeader.m_messageSize <= MESSAGE_MAX_LENGTH);
        memcpy(writePosition,&olsrMessageCache,olsrMessageCache.m_messageHeader.m_messageSize);
        writePosition += olsrMessageCache.m_messageHeader.m_messageSize;
        hasMessageCache = false;
      }
      while(xQueueReceive(g_olsrSendQueue, &olsrMessageCache, 700)){
        count++;
        configASSERT(olsrMessageCache.m_messageHeader.m_messageSize <= MESSAGE_MAX_LENGTH);
        if(0==olsrMessageCache.m_messageHeader.m_timeToLive) break;
        if(writePosition+olsrMessageCache.m_messageHeader.m_messageSize-(uint8_t *)messages>MESSAGE_MAX_LENGTH){
          hasMessageCache = true;
          break;
        }else{
          memcpy(writePosition,&olsrMessageCache,olsrMessageCache.m_messageHeader.m_messageSize);
          writePosition += olsrMessageCache.m_messageHeader.m_messageSize;
        }
      }
      if(writePosition-(uint8_t *)olsrPacket==sizeof(olsrPacketHeader_t)) {
        vTaskDelay(20);
        continue;
      }
      olsrPacket->m_packetHeader.m_packetLength = writePosition-(uint8_t *)olsrPacket;
      olsrPacket->m_packetHeader.m_packetSeq++;
      //transmit
      dwNewTransmit(dwm);
      dwSetDefaults(dwm);
      dwWaitForResponse(dwm, true);
      dwReceivePermanently(dwm, true);
      dwSetData(dwm, (uint8_t *)&dwPacket,MAC802154_HEADER_LENGTH+olsrPacket->m_packetHeader.m_packetLength);
      dwStartTransmit(dwm);
      DEBUG_PRINT_OLSR_SEND("send successful\n");
      count = 0;
      vTaskDelay(20);
    }
}
void olsrRecvTask(void *ptr){
    DEBUG_PRINT_OLSR_RECEIVE("RECV TASK START\n");
    static packet_t recvPacket;
    while(true){
      while(xQueueReceive(g_olsrRecvQueue,&recvPacket,0)){
        DEBUG_PRINT_OLSR_RECEIVE("RECV FROM QUEUE\n");
        olsrPacketDispatch(&recvPacket);
        olsr_routing_table_compute();
      }
      vTaskDelay(500);
    }
}
