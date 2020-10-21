#ifndef __OLSR_PACKET_H__
#define __OLSR_PACKET_H__

#include "mac.h"

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

#define maxPacketLength 125 //got this value by testing
#define payLoadMaxSize 104 //(maxPacketLength-MAC802154_HEADER_LENGTH)
#define messageMaxSize 100 //(payLoadMaxSize-sizeof(olsr_packet_hdr_t))
#define msgContMaxSize  84 //(messageMaxSize-sizeof(olsr_message_hdr_t))
#define tcAddrMaxCount (msgContMaxSize-2)/sizeof(olsr_topology_message_body_unit)
#define timeStampMaxCount  9 //((messageMaxSize-sizeof(olsr_ts_message_hdr_t))/sizeof(olsr_ts_message_bodyunit_t))

typedef enum 
{
    NOT_NEIGH=0,
    SYM_NEIGH = 1,
    MPR_NEIGH = 2,
}neighbor_type_t;
typedef struct{
    uint16_t olsr_p_length;
    uint16_t seq;
} __attribute__((packed)) olsr_packet_hdr_t;//4

typedef struct{
    olsr_packet_hdr_t header;
    uint8_t content[messageMaxSize]; 
    //int content_size;
} __attribute__((packed)) olsr_packet_t;

typedef enum{
    HELLO_MESSAGE = 1,
    TC_MESSAGE = 2,
    DATA_MESSAGE = 3,
    TS_MESSAGE= 4,
} message_type_t;
//1
typedef struct{
   message_type_t m_messageType;
   uint8_t m_vtime;
   uint16_t m_messageSize;
   uint16_t sourceAddr;
   uint16_t midAddr;
   uint16_t destAddr;
   uint16_t reserved;
   uint8_t m_timeTolive;
   uint8_t m_hopCount;
   uint16_t m_messageSeq;
} __attribute__((packed)) olsr_message_hdr_t; //16bytes

//message 
typedef struct
{
    olsr_message_hdr_t header;
    uint8_t content[messageMaxSize-sizeof(olsr_message_hdr_t)];
    //int content_size;
} __attribute__((packed)) olsr_message_t;

//hello message
typedef struct{
    uint16_t reserved;
    uint8_t Htime;
    uint8_t willingness;
} __attribute__((packed)) olsr_hello_message_hdr_t;

typedef struct{
    uint8_t link_code;
    uint8_t reserved;
    uint16_t size;
} __attribute__((packed)) olsr_link_message_hdr_t;

typedef struct{
    olsr_hello_message_hdr_t  hello_header;
    olsr_link_message_hdr_t   link_header;
    uint16_t address[10];
} __attribute__((packed)) olsr_hello_message_t;

//tc
typedef struct 
{
    /* data */
    uint16_t address;
    #ifdef DIS_OLSR
    uint16_t distance;
    #endif
} __attribute__((packed)) olsr_topology_message_body_unit;

typedef struct 
{
    /* data */
    uint16_t ansn;
    //42
    olsr_topology_message_body_unit content[tcAddrMaxCount];
} __attribute__((packed)) olsr_topology_message_t;


//time stamp (ts) message
typedef struct{
   message_type_t m_messageType;
   uint8_t dwtime_high8;
   uint16_t m_messageSize;
   uint16_t sourceAddr;
   uint16_t m_messageSeq;
   uint16_t velocity;//in cm
   uint32_t dwtime_low32;
} __attribute__((packed)) olsr_ts_message_hdr_t; //14

typedef struct { 
   uint16_t tsAddr;
   uint16_t m_messageSeq;
   uint32_t dwtime_low32;
   uint8_t dwtime_high8;
} __attribute__((packed)) olsr_ts_message_bodyunit_t;//9

typedef struct {
    olsr_ts_message_hdr_t ts_header;
    olsr_ts_message_bodyunit_t content[timeStampMaxCount];
} __attribute__((packed)) olsr_ts_message_t;


//link type
typedef enum
{
  UNSPEC_LINK = 0,
  ASYM_LINK   = 1,
  SYM_LINK    = 2,
  LOST_LINK   = 3,
} link_type_t;

inline uint8_t
olsr_link_code(link_type_t lt, neighbor_type_t nt)
{
  return ((nt & 0x3) << 2) | (lt & 0x3);
}

inline link_type_t
olsr_link_type(uint8_t link_code)
{
  return link_code & 0x3;
}

inline neighbor_type_t
olsr_neighbor_type(uint8_t link_code)
{
  return (link_code >> 2) & 0x3;
}

#endif //__OLSR_PACKET_H__
