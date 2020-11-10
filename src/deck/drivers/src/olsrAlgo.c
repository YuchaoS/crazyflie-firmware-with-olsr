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
#define TC_INTERVAL 500
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
void olsr_tc_process(const olsrMessage_t* tc_message){

}
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
static void addNeighborTuple(const olsrNeighborTuple_t* tuple)
{
  olsrInsertToNeighborSet(tuple);
  incrementAnsn();
}

static void addTwoHopNeighborTuple(const olsrTwoHopNeighborTuple_t* tuple)
{
  olsrInsertToTwoHopNeighborSet(tuple);
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
  setIndex_t neighborTuple = olsrFindNeighborByAddr(tuple->m_neighborAddr);
  if(neighborTuple==-1)
    {
      linkTupleAdded(tuple,willingness);
      neighborTuple = olsrFindNeighborByAddr(tuple->m_neighborAddr);
    }
  
  if(neighborTuple!=-1)
    {
      bool hasSymmetricLink = false;
      setIndex_t linkTupleIter = olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY];
      while(linkTupleIter!=-1)
        {
          if(olsrLinkSet[linkTupleIter].data.m_neighborAddr == tuple->m_neighborAddr &&\
          olsrLinkSet[linkTupleIter].data.m_symTime >= xTaskGetTickCount())
            {
              hasSymmetricLink = true;
              break;
            }
          linkTupleIter = olsrLinkSet[linkTupleIter].next;
        }
      if(hasSymmetricLink)
        {
          olsrNeighborSet[neighborTuple].data.m_status = STATUS_SYM;
        }
      else
        {
          olsrNeighborSet[neighborTuple].data.m_status = STATUS_NOT_SYM;
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
  setIndex_t linkTuple = olsrFindInLinkByAddr(helloMsg->m_messageHeader.m_originatorAddress);
  olsrTime_t now = xTaskGetTickCount();
  bool created = false, updated = false;

  if(linkTuple == -1)
    {
      olsrLinkTuple_t newLinkTuple;
      newLinkTuple.m_localAddr = myAddress;
      newLinkTuple.m_neighborAddr = helloMsg->m_messageHeader.m_originatorAddress;
      newLinkTuple.m_symTime = now - M2T(1000);
      newLinkTuple.m_expirationTime = now + helloMsg->m_messageHeader.m_vTime*1000;
      linkTuple = olsrInsertToLinkSet(&newLinkTuple);
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
  olsrLinkSet[linkTuple].data.m_asymTime = now +helloMsg->m_messageHeader.m_vTime*1000;
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
              olsrLinkSet[linkTuple].data.m_symTime = now - M2T(1000);
              updated = true;
            }
          else if(lt == OLSR_SYM_LINK ||lt == OLSR_ASYM_LINK)
            {
              olsrLinkSet[linkTuple].data.m_symTime = now +helloMsg->m_messageHeader.m_vTime*1000;
              olsrLinkSet[linkTuple].data.m_expirationTime = olsrLinkSet[linkTuple].data.m_symTime+OLSR_NEIGHB_HOLD_TIME*1000;
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
  olsrLinkSet[linkTuple].data.m_expirationTime = olsrLinkSet[linkTuple].data.m_asymTime > \
                                                olsrLinkSet[linkTuple].data.m_expirationTime? \
                                                olsrLinkSet[linkTuple].data.m_asymTime:\
                                                olsrLinkSet[linkTuple].data.m_expirationTime;
  xSemaphoreTake(olsrNeighborFullSetLock,portMAX_DELAY);
  if(updated)
    {
      linkTupleUpdated(&olsrLinkSet[linkTuple].data,helloMsgBody->m_helloHeader.m_willingness);
    }
  if(created)
    {
      linkTupleAdded(&olsrLinkSet[linkTuple].data,helloMsgBody->m_helloHeader.m_willingness); // same addr?
      //del
    }
  xSemaphoreGive(olsrNeighborFullSetLock); 
  xSemaphoreGive(olsrLinkFullSetLock); //Seamphore Give
}

void populateNeighborSet(const olsrMessage_t* helloMsg)
{
  xSemaphoreTake(olsrNeighborFullSetLock,portMAX_DELAY);
  setIndex_t nbTuple = olsrFindNeighborByAddr(helloMsg->m_messageHeader.m_originatorAddress);
  if(nbTuple != -1)
    {
      olsrNeighborSet[nbTuple].data.m_willingness = ((olsrHelloMessageHeader_t *)helloMsg->m_messagePayload)->m_willingness;
    }
  xSemaphoreGive(olsrNeighborFullSetLock); 
}

void PopulateTwoHopNeighborSet(const olsrMessage_t* helloMsg)
{
  xSemaphoreTake(olsrLinkFullSetLock,portMAX_DELAY);
  xSemaphoreTake(olsrTwoHopNeighborFullSetLock,portMAX_DELAY);
  olsrTime_t now = xTaskGetTickCount();
  olsrAddr_t sender = helloMsg->m_messageHeader.m_originatorAddress;
  setIndex_t linkTuple = olsrFindInLinkByAddr(sender);
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
              setIndex_t twoHopNeighborTuple = olsrFindTwoHopNeighborTuple(sender,candidate);
              if(twoHopNeighborTuple != -1)
                {
                  olsrTwoHopNeighborSet[twoHopNeighborTuple].data.m_expirationTime = now+\
                                                    helloMsg->m_messageHeader.m_vTime*1000;
                }
              else
                {
                  olsrTwoHopNeighborTuple_t newTuple;
                  newTuple.m_neighborAddr = sender;
                  newTuple.m_twoHopNeighborAddr = candidate;
                  newTuple.m_expirationTime = now+helloMsg->m_messageHeader.m_vTime*1000;
                  addTwoHopNeighborTuple(&newTuple);
                }
              
            }
          else if(nbType == OLSR_NOT_NEIGH)
            {
              olsrEraseTwoHopNeighborTuple(sender,candidate);
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

void olsrProcessHello(const olsrMessage_t* helloMsg)
{
  linkSensing(helloMsg);
  populateNeighborSet(helloMsg);
  PopulateTwoHopNeighborSet(helloMsg);
  // MprComputation();

}
//

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
            // olsr_process_hello_message(olsr_message);
            olsrProcessHello(olsrMessage);
            break;
        case TC_MESSAGE:
            DEBUG_PRINT_OLSR_RECEIVE("TC_MESSAGE\n");
            // olsr_tc_process(olsr_message);
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
  msg.m_messageHeader.m_relayAddress = 0;  
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
  setIndex_t linkTupleIndex = olsrSetIndexEntry[LINK_SET_T][FULL_ENTRY];
  olsrTime_t now = xTaskGetTickCount();
  while(linkTupleIndex!=-1)
    {
      if(!(olsrLinkSet[linkTupleIndex].data.m_localAddr == myAddress &&\
      olsrLinkSet[linkTupleIndex].data.m_expirationTime >= now))
        {
          linkTupleIndex = olsrLinkSet[linkTupleIndex].next;
          continue;
        }
      uint8_t linkType, nbType = 0xff;

      if(olsrLinkSet[linkTupleIndex].data.m_symTime>=now)
        {
          linkType = OLSR_SYM_LINK;//2
        }
      else if(olsrLinkSet[linkTupleIndex].data.m_asymTime>=now)
        {
          linkType = OLSR_ASYM_LINK;//1
        }
      else
        {
          linkType = OLSR_LOST_LINK;//3
        }
      if(olsrFindMprByAddr(olsrLinkSet[linkTupleIndex].data.m_neighborAddr))
        {
           nbType = OLSR_MPR_NEIGH;//2
        }
      else
        {
          bool ok = false;
          setIndex_t neighborTupleIndex = olsrSetIndexEntry[NEIGHBOR_SET_T][FULL_ENTRY];
          while(neighborTupleIndex!=-1)
            {
              if(olsrNeighborSet[neighborTupleIndex].data.m_neighborAddr ==\
              olsrLinkSet[linkTupleIndex].data.m_neighborAddr) // this linkTuple Addr is in NighborSet
                {
                  if(olsrNeighborSet[neighborTupleIndex].data.m_status == STATUS_SYM)
                    {
                      nbType = OLSR_SYM_NEIGH; //is a sym neighbor
                    }
                  else if(olsrNeighborSet[neighborTupleIndex].data.m_status == STATUS_NOT_SYM)
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
              linkTupleIndex = olsrLinkSet[linkTupleIndex].next;
              continue;
            }
        }
          olsrLinkMessage_t linkMessage;
          linkMessage.m_linkCode = (linkType & 0x03) | ((nbType << 2) & 0x0f);
          linkMessage.m_addressUsedSize = 1;
          linkMessage.m_addresses = olsrLinkSet[linkTupleIndex].data.m_neighborAddr;
          if(helloMessage.m_helloHeader.m_linkMessageNumber==LINK_MESSAGE_MAX_NUM) break;
          helloMessage.m_linkMessage[helloMessage.m_helloHeader.m_linkMessageNumber++] = linkMessage;
          linkTupleIndex = olsrLinkSet[linkTupleIndex].next;
    }
  xSemaphoreGive(olsrLinkFullSetLock);
  uint16_t writeSize = sizeof(olsrHelloMessageHeader_t)+helloMessage.m_helloHeader.m_linkMessageNumber*\
                       sizeof(olsrLinkMessage_t);
  msg.m_messageHeader.m_messageSize +=  writeSize;                 
  memcpy(msg.m_messagePayload,&helloMessage,writeSize);
  xQueueSend(g_olsrSendQueue,&msg,portMAX_DELAY);
}


//process hello

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
void olsr_send_tc_to_queue(){
    DEBUG_PRINT_OLSR_SYSTEM("TC_SEND TO QUEUE\n");
}
void olsr_send_ts_to_queue(){
    DEBUG_PRINT_OLSR_SEND("TS_SEND TO QUEUE\n");
}

void olsr_tc_task(void *ptr){
    while(true)
    {    
        if(true){//USE 
            olsr_send_tc_to_queue();
        }  
        vTaskDelay(M2T(TC_INTERVAL)); 
    }
}
void olsr_ts_task(void *ptr){
    while(true)
    {    
        olsr_send_ts_to_queue();
        vTaskDelay(M2T(TS_INTERVAL)); 
    }
}
void olsrSendTask(void *ptr)
{
    //pointer initialize
    packet_t dwPacket = {0};
    bool hasMessageCache =false;
    olsrMessage_t olsrMessageCache = {0};
  	MAC80215_PACKET_INIT(dwPacket, MAC802154_TYPE_OLSR)
    dwm = (dwDevice_t *)ptr; // can remove
    olsrPacket_t *olsrPacket = (olsrPacket_t *)dwPacket.payload;
    olsrMessage_t *messages;
    //task loop
    DEBUG_PRINT_OLSR_SEND("top:dwpacket:%u\n",(unsigned int)&dwPacket);
    DEBUG_PRINT_OLSR_SEND("top:olsr_packet:%u\n",(unsigned int)olsrPacket);
    int count = 0;
    while(true){
      messages = (olsrMessage_t *)olsrPacket->m_packetPayload;
      uint8_t *writePosition = (uint8_t *)messages;
      DEBUG_PRINT_OLSR_SEND("top:write_position:%u\n",(unsigned int)writePosition);
      if(hasMessageCache){
        configASSERT(olsrMessageCache.m_messageHeader.m_messageSize <= MESSAGE_MAX_LENGTH);
        memcpy(writePosition,&olsrMessageCache,olsrMessageCache.m_messageHeader.m_messageSize);
        writePosition += olsrMessageCache.m_messageHeader.m_messageSize;
        hasMessageCache = false;
        DEBUG_PRINT_OLSR_SEND("if1:write_position:%u\n",(unsigned int)writePosition);
      }
      while(xQueueReceive(g_olsrSendQueue, &olsrMessageCache, 700)){
        count++;
        configASSERT(olsrMessageCache.m_messageHeader.m_messageSize <= MESSAGE_MAX_LENGTH);
        if(0==olsrMessageCache.m_messageHeader.m_timeToLive) break;
        if(writePosition+olsrMessageCache.m_messageHeader.m_messageSize-(uint8_t *)messages>MESSAGE_MAX_LENGTH){
          hasMessageCache = true;
          break;
        }else{
          DEBUG_PRINT_OLSR_SEND("this [%d]msg:%d\n",count,olsrMessageCache.m_messageHeader.m_messageSize);
          memcpy(writePosition,&olsrMessageCache,olsrMessageCache.m_messageHeader.m_messageSize);
          writePosition += olsrMessageCache.m_messageHeader.m_messageSize;
         DEBUG_PRINT_OLSR_SEND("else:write_position:%u\n",(unsigned int)writePosition);
        }
      }
      DEBUG_PRINT_OLSR_SEND("continue:write_position:%u\n",(unsigned int)(writePosition-(uint8_t *)olsrPacket));
      if(writePosition-(uint8_t *)olsrPacket==sizeof(olsrPacketHeader_t)) {
        vTaskDelay(20);
        continue;
      }
      olsrPacket->m_packetHeader.m_packetLength = writePosition-(uint8_t *)olsrPacket;
      olsrPacket->m_packetHeader.m_packetSeq++;
      DEBUG_PRINT_OLSR_SEND("bottom:olsr_p_length:%d\n",olsrPacket->m_packetHeader.m_packetLength);
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
