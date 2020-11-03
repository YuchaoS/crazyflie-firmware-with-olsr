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
#define OLSR_NEIGHB_HOLD_TIME 3*OLSR_HELLO_INTERVAL
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
static SemaphoreHandle_t olsrMessageSeqLock;
packet_t rxPacket;
//TODO define packet and message struct once, save space
//debugging, to be deleted
//TODO delete testDataLength2send


//rxcallback
void device_init(dwDevice_t *dev){
  dwm = dev;
  olsrMessageSeqLock = xSemaphoreCreateMutex();
}
static uint16_t getSeqNumber()
{
  xSemaphoreTake(olsrMessageSeqLock,portMAX_DELAY);
  uint16_t retVal = g_staticMessageSeq++;
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
void olsr_tc_process(const olsr_message_t* tc_message){

}
void olsr_tc_forward(olsr_message_t* tc_message){

}
void olsr_ts_process(const olsr_message_t* ts_msg){

}

//

//switch to tc|hello|ts process
void olsr_packet_dispatch
(const packet_t* rxPacket){
    DEBUG_PRINT_OLSR_SYSTEM("PACKET_DISPATCH\n");  
    olsr_packet_t* olsr_packet = (olsr_packet_t *)rxPacket->payload;
    //need to add a condition whether recvive a packet from self
    uint16_t olsr_packet_sz = olsr_packet->header.olsr_p_length;
    uint16_t index = sizeof(olsr_packet_hdr_t);
    uint8_t* olsr_message_idx = olsr_packet->content;
    olsr_message_t* olsr_message = (olsr_message_t*)olsr_message_idx;
    DEBUG_PRINT_OLSR_RECEIVE("RECV PACKET LEN: %d\n", olsr_packet_sz);  
    while(index<olsr_packet_sz){
        olsr_message_hdr_t* olsr_message_header = (olsr_message_hdr_t*)olsr_message;
        message_type_t m_type = olsr_message_header->m_messageType;
				#ifdef DEBUG_OLSR_RECEIVE
        DEBUG_PRINT_OLSR_RECEIVE("HDR size\t%d\n", olsr_message_header->m_messageSize);
        DEBUG_PRINT_OLSR_RECEIVE("HDR seq\t%d\n", olsr_message_header->m_messageSeq);
				#endif //DEBUG_OLSR_RECEIVE
        switch (m_type) {
        case HELLO_MESSAGE:
            DEBUG_PRINT_OLSR_RECEIVE("HELLO_MESSAGE\n");
            // olsr_process_hello_message(olsr_message);
            break;
        case TC_MESSAGE:
            DEBUG_PRINT_OLSR_RECEIVE("TC_MESSAGE\n");
            olsr_tc_process(olsr_message);
            olsr_tc_forward(olsr_message);
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
        index+=olsr_message_header->m_messageSize;
        olsr_message_idx+=olsr_message_header->m_messageSize;
        olsr_message=(olsr_message_t*)olsr_message_idx;
    }
}

//send related function
void olsr_generate_ts(olsr_message_t *ts_msg)
{
}

void olsrSendHello()
{
  olsrMessage_t helloMessage;
  //message header initial
  helloMessage.m_messageHeader.m_messageType = HELLO_MESSAGE;
  helloMessage.m_messageHeader.m_vTime = OLSR_NEIGHB_HOLD_TIME;
  helloMessage.m_messageSize = sizeof(olsrMessageHeader_t);
  helloMessage.m_messageHeader.m_originatorAddress = myAddress;
  helloMessage.m_messageHeader.m_destinationAddress = 0;
  helloMessage.m_messageHeader.m_relayAddress = 0;  
  helloMessage.m_messageHeader.m_timeToLive = 0xff;
  helloMessage.m_messageHeader.m_hopCount = 0;
  helloMessage.m_messageHeader.m_messageSeq = getSeqNumber();
  olsrHelloMessageHeader_t helloMessageHeader;
  helloMessageHeader.m_hTime = HELLO_INTERVAL;
  helloMessageHeader.m_willingness = WILL_ALWAYS;

  //loop
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
      if(olsrFindMprByAddr(olsrLinkSet[linkTupleIndex].data.m_localAddr))
        {
           nb_type = OLSR_MPR_NEIGH;//2
        }
      else
        {
          
        }
      
    }

}
static void olsr_generate_tc(olsr_message_t *tc_message){
  //write header
}

//process hello

// all task defination
void olsrHelloTask(void *ptr){
    while (true)
    {
        /* code */
        DEBUG_PRINT_OLSR_SEND("HELLO_SEND TO QUEUE\n");
        olsrSendHello();
        vTaskDelay(M2T(HELLO_INTERVAL));
    }
}
void olsr_send_tc_to_queue(){
    olsr_message_t t;
    olsr_generate_tc(&t);
    xQueueSend(g_olsrSendQueue,&t,portMAX_DELAY);
    DEBUG_PRINT_OLSR_SYSTEM("TC_SEND TO QUEUE\n");
}
void olsr_send_ts_to_queue(){
    olsr_message_t ts_msg={0};
    olsr_generate_ts(&ts_msg);
    xQueueSend(g_olsrSendQueue,&ts_msg,portMAX_DELAY);
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
void olsr_send_task(void *ptr)
{
    //pointer initialize
    packet_t dwpacket = {0};
    bool has_olsr_message_cache =false;
    olsr_message_t olsr_message_cache = {0};
  	MAC80215_PACKET_INIT(dwpacket, MAC802154_TYPE_OLSR)
    dwm = (dwDevice_t *)ptr;
    olsr_packet_t *olsr_packet = (olsr_packet_t *)dwpacket.payload;
    olsr_message_t *messages;
    //task loop
    DEBUG_PRINT_OLSR_SEND("top:dwpacket:%u\n",(unsigned int)&dwpacket);
    DEBUG_PRINT_OLSR_SEND("top:olsr_packet:%u\n",(unsigned int)olsr_packet);
    int count = 0;
    while(true){
      messages = (olsr_message_t *)olsr_packet->content;
      uint8_t *write_position = (uint8_t *)messages;
      DEBUG_PRINT_OLSR_SEND("top:write_position:%u\n",(unsigned int)write_position);
      if(has_olsr_message_cache){
        configASSERT(olsr_message_cache.header.m_messageSize <= messageMaxSize);
        memcpy(write_position,&olsr_message_cache,olsr_message_cache.header.m_messageSize);
        write_position += olsr_message_cache.header.m_messageSize;
        has_olsr_message_cache = false;
         DEBUG_PRINT_OLSR_SEND("if1:write_position:%u\n",(unsigned int)write_position);
      }
      while(xQueueReceive(g_olsrSendQueue, &olsr_message_cache, 700)){
        count++;
        configASSERT(olsr_message_cache.header.m_messageSize <= messageMaxSize);
        if(0==olsr_message_cache.header.m_timeTolive) break;
        if(write_position+olsr_message_cache.header.m_messageSize-(uint8_t *)messages>messageMaxSize){
          has_olsr_message_cache = true;
          break;
        }else{
          DEBUG_PRINT_OLSR_SEND("this [%d]msg:%d\n",count,olsr_message_cache.header.m_messageSize);
          memcpy(write_position,&olsr_message_cache,olsr_message_cache.header.m_messageSize);
          write_position += olsr_message_cache.header.m_messageSize;
         DEBUG_PRINT_OLSR_SEND("else:write_position:%u\n",(unsigned int)write_position);
        }
      }
      DEBUG_PRINT_OLSR_SEND("continue:write_position:%u\n",(unsigned int)(write_position-(uint8_t *)olsr_packet));
      if(write_position-(uint8_t *)olsr_packet==sizeof(olsr_packet_hdr_t)) {
        vTaskDelay(20);
        continue;
      }
      olsr_packet->header.olsr_p_length = write_position-(uint8_t *)olsr_packet;
      olsr_packet->header.seq++;
      DEBUG_PRINT_OLSR_SEND("bottom:olsr_p_length:%d\n",olsr_packet->header.olsr_p_length);
      //transmit
      dwNewTransmit(dwm);
      dwSetDefaults(dwm);
      dwWaitForResponse(dwm, true);
      dwReceivePermanently(dwm, true);
      dwSetData(dwm, (uint8_t *)&dwpacket,MAC802154_HEADER_LENGTH+olsr_packet->header.olsr_p_length);
      dwStartTransmit(dwm);
      DEBUG_PRINT_OLSR_SEND("send successful\n");
      count = 0;
      vTaskDelay(20);
    }
}
void olsr_recv_task(void *ptr){
    DEBUG_PRINT_OLSR_RECEIVE("RECV TASK START\n");
    static packet_t recvPacket;
    while(true){
      while(xQueueReceive(g_olsrRecvQueue,&recvPacket,0)){
        DEBUG_PRINT_OLSR_RECEIVE("RECV FROM QUEUE\n");
        olsr_packet_dispatch(&recvPacket);
        olsr_routing_table_compute();
      }
      vTaskDelay(500);
    }
}
