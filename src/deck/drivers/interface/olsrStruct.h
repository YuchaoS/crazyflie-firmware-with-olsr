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
********************Set*******************
*/

#endif //__OLSR_STRUCT_H__