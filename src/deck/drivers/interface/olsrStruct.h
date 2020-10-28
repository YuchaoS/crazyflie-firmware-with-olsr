#ifndef __OLSR_STRUCT_H__
#define __OLSR_STRUCT_H__
#include "FreeRTOS.h"
#include<queue.h>
#include"locodeck.h"
#include"FreeRTOS.h"
/*
*********************Recv&SendQueue*************************
*/
xQueueHandle g_olsrSendQueue;
xQueueHandle g_olsrRecvQueue;
void olsrStructInitAll(dwDevice_t *dev);

/*
*********************NeighborSet*************************
*/
typedef portTickType olsrTime_t;
typedef uint16_t olsrAddr_t;
typedef uint16_t olsrDist_t;
typedef uint8_t setIndex_t;
#define TOPOLOGY_SET_SIZE 30
#define MPR_SET_SIZE 30
#define DUPLICATE_SET_SIZE 30
#define TIMESTAMP_SET_SIZE 30
#define MPR_SELECTOR_SET_SIZE 30
#define NEIGHBOR_SET_SIZE 30

typedef enum
{
  WILL_NEVER   = 0,
  WILL_LOW     = 1,
  WILL_DEFAULT = 3,
  WILL_HIGH    = 6,
  WILL_ALWAYS  = 7,
} olsrWillingness_t;

typedef enum 
{
  NOT_NEIGH=0,
  SYM_NEIGH = 1,
  MPR_NEIGH = 2,
} olsrNeighborType_t;

typedef struct
{
  olsrAddr_t m_neighborAddr;
  olsrNeighborType_t m_status;
  bool m_isAdvertised;
  olsrWillingness_t m_willingness;
} olsrNeighborTuple_t;

/*
*********************Neighbor2Set*************************
*/
typedef struct
{
  olsrAddr_t m_neighborAddr;
  olsrAddr_t m_twoHopNeighborAddr;
  olsrTime_t m_expirationTime; //need fix name
} olsrTwoHopNeighborTuple_t;

/*
*********************LinkSet*************************
*/
typedef struct
{
  olsrAddr_t m_localAddr;
  olsrAddr_t m_neighborAddr;
  olsrTime_t m_symTime;
  olsrTime_t m_asymTime;
  olsrTime_t m_expirationTime;
} olsrLinkTuple_t;


/*
*********************MprSet*************************
*/
typedef struct 
{
  olsrAddr_t m_addr;
} olsrMprTuple_t;

/*
*********************MprSelectorSet*************************
*/
typedef struct 
{
  olsrAddr_t m_addr;
  olsrTime_t m_expirationTime;
} olsrMprSelectorTuple_t;
/*
*********************TopologySet*************************
*/
typedef struct 
{
  olsrAddr_t m_destAddr;
  olsrAddr_t m_lastAddr;
  uint16_t m_seqenceNumber;
  olsrDist_t m_distance;
  olsrTime_t m_expirationTime;
} olsrTopologyTuple_t;

/*
*********************DuplicateSet*************************
*/
typedef struct
{
  olsrAddr_t m_addr;
  uint16_t m_seqenceNumber;
  bool m_retransmitted;
  olsrTime_t m_expirationTime;
} olsrDuplicateTuple_t;

/*
*********************TimestampSet*************************
*/
typedef struct 
{
  uint16_t m_seqenceNumber;
  dwTime_t m_timestamp;
} olsrTimestampTuple_t;

/*
*********************RoutingTable*************************
*/
typedef struct 
{
  olsrAddr_t m_destAddr;
  olsrAddr_t m_nextAddr;
  olsrDist_t m_distance;
  olsrTime_t m_expirationTime;
} olsrRoutingTuple_t;

/*
********************Set Define*******************
*/


typedef struct 
{
  olsrTopologyTuple_t data;
  setIndex_t next;
} olsrTopologySetItem_t;

olsrTopologySetItem_t olsrTopologySet[TOPOLOGY_SET_SIZE];

typedef struct 
{
  olsrNeighborTuple_t data;
  setIndex_t next;
} olsrNeighborSetItem_t;

olsrNeighborSetItem_t olsrNeighborSet[NEIGHBOR_SET_SIZE];

typedef struct 
{
  olsrDuplicateTuple_t data;
  setIndex_t next;
} olsrDuplicateSetItem_t;

olsrDuplicateSetItem_t olsrDuplicateSet[DUPLICATE_SET_SIZE];

typedef struct 
{
  olsrMprTuple_t data;
  setIndex_t next;
} olsrMprSetItem_t;

olsrMprSetItem_t olsrMprSet[MPR_SET_SIZE];

typedef struct 
{
  olsrTimestampTuple_t data;
  setIndex_t next;
} olsrTimestampSetItem_t;

olsrTimestampSetItem_t olsrTimestampSet[TIMESTAMP_SET_SIZE];






#endif //__OLSR_STRUCT_H__