#ifndef __OLSR_STRUCT_H__
#define __OLSR_STRUCT_H__
#include "FreeRTOS.h"
#include "semphr.h"
#include <queue.h>
#include"locodeck.h"
#include <string.h>

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
typedef short olsrDist_t;
typedef short setIndex_t;


#define OLSR_SETS_NUM 8
#define TOPOLOGY_SET_T 0
#define MPR_SET_T 1
#define NEIGHBOR_SET_T 2
#define DUPLICATE_SET_T 3
#define MPR_SELECT_SET_T 4
#define TWO_HOP_NEIGHBOR_SET_T 5
#define TIMESTAMP_SET_T 6
#define LINK_SET_T 7


#define TOPOLOGY_SET_SIZE 30
#define LINK_SET_SIZE 30
#define MPR_SET_SIZE 30
#define DUPLICATE_SET_SIZE 30
#define TIMESTAMP_SET_SIZE 30
#define MPR_SELECTOR_SET_SIZE 30
#define NEIGHBOR_SET_SIZE 30
#define TWO_HOP_NEIGHBOR_SET_SIZE 30

SemaphoreHandle_t olsrNeighborEmptySetLock;
SemaphoreHandle_t olsrNeighborFullSetLock;
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
  STATUS_NOT_SYM = 0,
  STATUS_SYM = 1,
} olsrNeighborType_t;

typedef struct
{
  olsrAddr_t m_neighborAddr;
  olsrNeighborType_t m_status;
  bool m_isAdvertised;
  olsrWillingness_t m_willingness;
} olsrNeighborTuple_t;

typedef struct 
{
  olsrNeighborTuple_t data;
  setIndex_t next;
} olsrNeighborSetItem_t;
/*
*********************Neighbor2Set*************************
*/
SemaphoreHandle_t olsrTwoHopNeighborEmptySetLock;
SemaphoreHandle_t olsrTwoHopNeighborFullSetLock;
typedef struct
{
  olsrAddr_t m_neighborAddr;
  olsrAddr_t m_twoHopNeighborAddr;
  olsrTime_t m_expirationTime; //need fix name
} olsrTwoHopNeighborTuple_t;

typedef struct
{
  olsrTwoHopNeighborTuple_t data;
  setIndex_t next;
} olsrTwoHopNeighborSetItem_t;


/*
*********************LinkSet*************************
*/
SemaphoreHandle_t olsrLinkEmptySetLock;
SemaphoreHandle_t olsrLinkFullSetLock;
typedef struct
{
  olsrAddr_t m_localAddr;
  olsrAddr_t m_neighborAddr;
  /// The link is considered bidirectional until this time.
  olsrTime_t m_symTime;
  /// The link is considered unidirectional until this time.
  olsrTime_t m_asymTime;
  olsrTime_t m_expirationTime;
} olsrLinkTuple_t;

typedef struct 
{
  olsrLinkTuple_t data;
  setIndex_t next;
} olsrLinkSetItem_t;


/*
*********************MprSet*************************
*/

SemaphoreHandle_t olsrMprEmptySetLock;
SemaphoreHandle_t olsrMprFullSetLock;
typedef struct 
{
  olsrAddr_t m_addr;
} olsrMprTuple_t;

typedef struct 
{
  olsrMprTuple_t data;
  setIndex_t next;
} olsrMprSetItem_t;
/*
*********************MprSelectorSet*************************
*/
typedef struct 
{
  olsrAddr_t m_addr;
  olsrTime_t m_expirationTime;
} olsrMprSelectorTuple_t;

typedef struct 
{
  olsrMprSelectorTuple_t data;
  setIndex_t next;
} olsrMprSelectorSetItem_t;

/*
*********************TopologySet*************************
*/
SemaphoreHandle_t olsrTopologyEmptySetLock;
SemaphoreHandle_t olsrTopologyFullSetLock;
typedef struct 
{
  olsrAddr_t m_destAddr;
  olsrAddr_t m_lastAddr;
  uint16_t m_seqenceNumber;
  olsrDist_t m_distance;
  olsrTime_t m_expirationTime;
} olsrTopologyTuple_t;

typedef struct 
{
  olsrTopologyTuple_t data;
  setIndex_t next;
} olsrTopologySetItem_t;
/*
*********************DuplicateSet*************************
*/
SemaphoreHandle_t olsrDuplicateEmptySetLock;
SemaphoreHandle_t olsrDuplicateFullSetLock;
typedef struct
{
  olsrAddr_t m_addr;
  uint16_t m_seqenceNumber;
  bool m_retransmitted;
  olsrTime_t m_expirationTime;
} olsrDuplicateTuple_t;

typedef struct 
{
  olsrDuplicateTuple_t data;
  setIndex_t next;
} olsrDuplicateSetItem_t;
/*
*********************TimestampSet*************************
*/

typedef struct 
{
  uint16_t m_seqenceNumber;
  dwTime_t m_timestamp;
} olsrTimestampTuple_t;

typedef struct 
{
  olsrTimestampTuple_t data;
  setIndex_t next;
} olsrTimestampSetItem_t;
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
********************Set Definition*******************
*/



typedef struct 
{
  olsrTopologySetItem_t setData[TOPOLOGY_SET_SIZE];
  setIndex_t freeQueueEntry;
  setIndex_t fullQueueEntry;
} olsrTopologySet_t;


typedef struct
{
  olsrLinkSetItem_t setData[LINK_SET_SIZE];
  setIndex_t freeQueueEntry;
  setIndex_t fullQueueEntry; 
} olsrLinkSet_t;


typedef struct 
{
  olsrNeighborSetItem_t setData[NEIGHBOR_SET_SIZE];
  setIndex_t freeQueueEntry;
  setIndex_t fullQueueEntry; 
} olsrNeighborSet_t;


typedef struct 
{
  olsrTwoHopNeighborSetItem_t setData[TWO_HOP_NEIGHBOR_SET_SIZE];
  setIndex_t freeQueueEntry;
  setIndex_t fullQueueEntry;
} olsrTwoHopNeighborSet_t;


typedef struct
{
  olsrDuplicateSetItem_t setData[DUPLICATE_SET_SIZE];
  setIndex_t freeQueueEntry;
  setIndex_t fullQueueEntry;
} olsrDuplicateSet_t;


typedef struct 
{
  olsrMprSetItem_t setData[MPR_SET_SIZE];
  setIndex_t freeQueueEntry;
  setIndex_t fullQueueEntry;
} olsrMprSet_t;


typedef struct 
{
  olsrMprSelectorSetItem_t setData[MPR_SELECTOR_SET_SIZE];
  setIndex_t freeQueueEntry;
  setIndex_t fullQueueEntry; 
} olsrMprSelectorSet_t;



typedef struct
{
  olsrTimestampSetItem_t setData[TIMESTAMP_SET_SIZE];
  setIndex_t freeQueueEntry;
  setIndex_t fullQueueEntry;
} olsrTimestampSet_t;


olsrTopologySet_t olsrTopologySet;

olsrNeighborSet_t olsrNeighborSet;

olsrLinkSet_t olsrLinkSet;

olsrTwoHopNeighborSet_t olsrTwoHopNeighborSet;

olsrDuplicateSet_t olsrDuplicateSet;

olsrMprSet_t olsrMprSet;

olsrMprSelectorSet_t olsrMprSelectorSet;

/*linkSet*/
setIndex_t olsrInsertToLinkSet(olsrLinkSet_t *linkSet, olsrLinkTuple_t *item);

setIndex_t olsrFindInLinkByAddr(olsrLinkSet_t *linkSet, const olsrAddr_t addr);

void olsrPrintLinkSet(olsrLinkSet_t *linkSet);

setIndex_t olsrFindSymLinkTuple(olsrLinkSet_t *linkSet,olsrAddr_t sender,olsrTime_t now);

/* twoHopNeighbor*/
void olsrTwoHopNeighborSetInit(olsrTwoHopNeighborSet_t *twoHopNeighborSet);

setIndex_t olsrFindTwoHopNeighborTuple(olsrTwoHopNeighborSet_t *twoHopNeighborSet,\
                                       olsrAddr_t neighborAddr,\
                                       olsrAddr_t twoHopNeighborAddr);

setIndex_t olsrInsertToTwoHopNeighborSet(olsrTwoHopNeighborSet_t* twoHopNeighborSet,\
                                        const olsrTwoHopNeighborTuple_t* tuple);

setIndex_t olsrEraseTwoHopNeighborTuple(olsrTwoHopNeighborSet_t* twoHopNeighborSet,\
                                  olsrAddr_t neighborAddr,\
                                  olsrAddr_t twoHopNeighborAddr);
setIndex_t olsrEraseTwoHopNeighborTupleByTuple(olsrTwoHopNeighborSet_t *twoHopNeighborSet,\
                                        olsrTwoHopNeighborTuple_t *tuple);

/* Neighbor*/
setIndex_t olsrFindNeighborByAddr(olsrNeighborSet_t* neighborSet,\
                                  olsrAddr_t addr);

setIndex_t olsrInsertToNeighborSet(olsrNeighborSet_t* neighborSet,\
                                  const olsrNeighborTuple_t* tuple);

void olsrNeighborSetInit(olsrNeighborSet_t *neighborSet);

void olsrPrintNeighborSet(olsrNeighborSet_t *neighborSet);

/*topologySet */
bool olsrInsertToTopologySet(olsrTopologySet_t *topologySet,\
                              olsrTopologyTuple_t *tcTuple);

setIndex_t olsrFindNewerTopologyTuple(olsrTopologySet_t *topologyset,\
                                      olsrAddr_t originator,\
                                      uint16_t ansn);

/*mpr*/
void olsrMprSetInit(olsrMprSet_t *mprSet);
bool olsrFindMprByAddr(olsrMprSet_t *mprSet,olsrAddr_t addr);
setIndex_t olsrInsertToMprSet(olsrMprSet_t *MprSet,olsrMprTuple_t *item);

/*ms*/
setIndex_t olsrInsertToMprSelectorSet(olsrMprSelectorSet_t *mprSelectorSet,\
                                      olsrMprSelectorTuple_t *item);

setIndex_t olsrFindInMprSelectorSet(olsrMprSelectorSet_t *mprSelectorSet, olsrAddr_t addr);

bool olsrMprSelectorSetIsEmpty();
#endif //__OLSR_STRUCT_H__