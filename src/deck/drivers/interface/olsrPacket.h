#ifndef __OLSR_PACKET_H__
#define __OLSR_PACKET_H__

#include "mac.h"
#include "olsrStruct.h"

/*
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |         Packet Length         |    Packet Sequence Number     |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |  Message Type |     Vtime     |         Message Size          |//HELLO_MESSAGE
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          Address1             |         Address2              |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          Address3             |         RESERVED              |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |  Time To Live |   Hop Count   |    Message Sequence Number    |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          Reserved             |     Htime     |  Willingness  |//HELLO
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |   Link Code   |   Reserved    |       Link Message Size       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          Address1             |         Address2              |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |   Link Code   |   Reserved    |       Link Message Size       |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          Address1             |         Address2              |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      :                            MESSAGE                            :
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |  Message Type |     Vtime     |         Message Size          |//TC_MESSAGE
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          Address1             |         Address2              |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          Address3             |         RESERVED              |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |  Time To Live |   Hop Count   |    Message Sequence Number    |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |              ANSN             |           Reserved            |//TC
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          Address1             |         distance1             |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          Address2             |         distance2             |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                                                               |
      :                            MESSAGE                            :
      |                                                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |  Message Type |    TS_SENT    |         Message Size          |//TIMESTAMP_MESSAGE
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          srcAddress           |    Message Sequence Number    |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          Veloctiy(cm)         |             TS_SENT           |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |             TS_SENT           |                               
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |            tsAddr1            |    Message Sequence Number    |     
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                        TS_RECV1                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |    TS_RECV1   |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |            tsAddr2            |    Message Sequence Number    |     
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                        TS_RECV2                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |    TS_RECV2   |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |  Message Type |     Vtime     |         Message Size          |//_MESSAGE
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          Address1             |         Address2              |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |          Address3             |         RESERVED              |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |  Time To Live |   Hop Count   |    Message Sequence Number    |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      :                                                               :
      :                                                               :
               (etc.)
 */

#define PACKET_MAX_LENGTH 125 //got this value by testing
#define PACKET_PAYLOAD_MAX_SIZE 104 //(maxPacketLength-MAC802154_HEADER_LENGTH)
#define MESSAGE_MAX_LENGTH 100 //(payLoadMaxSize-sizeof(olsr_packet_hdr_t))
#define MESSAGE_PAYLOAD_MAX_SIZE  84 //(messageMaxSize-sizeof(olsr_message_hdr_t))
#define LINK_ADDRESS_MAX_NUM 10
#define LINK_MESSAGE_MAX_NUM ((MESSAGE_PAYLOAD_MAX_SIZE-sizeof(olsrHelloMessageHeader_t))\
                              /sizeof(olsrLinkMessage_t))

typedef struct{
    uint16_t m_packetLength;
    uint16_t m_packetSeq;
} __attribute__((packed)) olsrPacketHeader_t;//4

typedef struct{
    olsrPacketHeader_t m_packetHeader;
    uint8_t m_packetPayload[PACKET_PAYLOAD_MAX_SIZE]; 
    //int content_size;
} __attribute__((packed)) olsrPacket_t;

typedef enum{
    HELLO_MESSAGE = 1,
    TC_MESSAGE = 2,
    DATA_MESSAGE = 3,
    TS_MESSAGE= 4,
} olsrMessageType_t;
//1
typedef struct{
   olsrMessageType_t m_messageType;
   uint8_t m_vTime; //The validity time.
   uint16_t m_messageSize;
   olsrAddr_t m_originatorAddress;
   olsrAddr_t m_relayAddress;
   olsrAddr_t m_destinationAddress;
   uint16_t m_reserved;
   uint8_t m_timeToLive;
   uint8_t m_hopCount;
   uint16_t m_messageSeq;
} __attribute__((packed)) olsrMessageHeader_t; //16bytes

//message 
typedef struct
{
    olsrMessageHeader_t m_messageHeader;
    uint8_t m_messagePayload[MESSAGE_MAX_LENGTH-sizeof(olsrMessageHeader_t)];
    //int content_size;
} __attribute__((packed)) olsrMessage_t;

//hello message
typedef struct{
    uint16_t m_reserved;
    uint8_t m_hTime;
    uint8_t m_willingness;
} __attribute__((packed)) olsrHelloMessageHeader_t;//4bytes

typedef struct{
    uint8_t m_linkCode;
    uint8_t m_reserved;
    uint16_t m_addressUsedSize;
    olsrAddr_t m_addresses[LINK_ADDRESS_MAX_NUM];
} __attribute__((packed)) olsrLinkMessage_t; //4+2*10bytes

typedef struct{
    olsrHelloMessageHeader_t  m_helloHeader;
    olsrLinkMessage_t m_linkMessage[LINK_MESSAGE_MAX_NUM];
} __attribute__((packed)) olsrHeloMessage_t;

//tc
// typedef struct 
// {
//     /* data */
//     uint16_t address;
//     #ifdef DIS_OLSR
//     uint16_t distance;
//     #endif
// } __attribute__((packed)) olsr_topology_message_body_unit;

// typedef struct 
// {
//     /* data */
//     uint16_t ansn;
//     //42
//     olsr_topology_message_body_unit content[tcAddrMaxCount];
// } __attribute__((packed)) olsr_topology_message_t;


// //time stamp (ts) message
// typedef struct{
//    message_type_t m_messageType;
//    uint8_t dwtime_high8;
//    uint16_t m_messageSize;
//    uint16_t sourceAddr;
//    uint16_t m_messageSeq;
//    uint16_t velocity;//in cm
//    uint32_t dwtime_low32;
// } __attribute__((packed)) olsr_ts_message_hdr_t; //14

// typedef struct { 
//    uint16_t tsAddr;
//    uint16_t m_messageSeq;
//    uint32_t dwtime_low32;
//    uint8_t dwtime_high8;
// } __attribute__((packed)) olsr_ts_message_bodyunit_t;//9

// typedef struct {
//     olsr_ts_message_hdr_t ts_header;
//     olsr_ts_message_bodyunit_t content[timeStampMaxCount];
// } __attribute__((packed)) olsr_ts_message_t;


// //link type
// typedef enum
// {
//   UNSPEC_LINK = 0,
//   ASYM_LINK   = 1,
//   SYM_LINK    = 2,
//   LOST_LINK   = 3,
// } link_type_t;

// inline uint8_t
// olsr_link_code(link_type_t lt, olsrNeighborType_t nt)
// {
//   return ((nt & 0x3) << 2) | (lt & 0x3);
// }

// inline link_type_t
// olsr_link_type(uint8_t link_code)
// {
//   return link_code & 0x3;
// }

// inline olsrNeighborType_t
// olsr_neighbor_type(uint8_t link_code)
// {
//   return (link_code >> 2) & 0x3;
// }

#endif //__OLSR_PACKET_H__
