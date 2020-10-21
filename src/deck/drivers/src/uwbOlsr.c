#define DEBUG_MODULE "OLSR"

#include "FreeRTOS.h"
#include "system.h"
#include "task.h"
#include<queue.h>
#include<string.h>
#include<semphr.h>
#include "locodeck.h"
#include "log.h"
#include "param.h"
#include "configblock.h"
#include "mac.h"
#include "olsrDebug.h"
#include "olsrAlgo.h"
#include "olsrStruct.h"


uint16_t myAddress ;

static void OLSR_SET_INIT(){
    DEBUG_PRINT_OLSR_SYSTEM("START OLSR_SET_INIT\n");

}
static void OLSR_QUEUE_INIT(){
    DEBUG_PRINT_OLSR_SYSTEM("OLSR_QUEUE_INIT\n");
    olsr_send_queue_init();
    olsr_recv_queue_init();
}
static void OLSR_ROUTE_TABLE_INIT(){
    DEBUG_PRINT_OLSR_SYSTEM("OLSR_ROUTE_TABLE_INIT\n");
}
static void OLSR_DEV_INIT(dwDevice_t *dev){
    device_init(dev);
}
static void OLSR_STRUCT_INIT(dwDevice_t *dev){
    DEBUG_PRINT_OLSR_SYSTEM("START OLSR_STRUCT_INIT\n");
    OLSR_QUEUE_INIT();
    OLSR_DEV_INIT(dev);
    OLSR_SET_INIT();
    OLSR_ROUTE_TABLE_INIT();
}
static void OLSR_HELLO_TASK_INIT(){
    DEBUG_PRINT_OLSR_SYSTEM("START_OLSR_HELLO_TASK_INIT\n");
    if(xTaskCreate(olsr_hello_task, "OLSR_HELLO", 5*configMINIMAL_STACK_SIZE, NULL,
                    LPS_DECK_TASK_PRI, NULL)==pdPASS){
                        DEBUG_PRINT_OLSR_SYSTEM("HELLO TASK CREATE SUCCESSFUL\n");
                    }else{
                        DEBUG_PRINT_OLSR_SYSTEM("HELLO TASK CREATE FAILD\n");
                    };
}
static void OLSR_TC_TASK_INIT(){
    DEBUG_PRINT_OLSR_SYSTEM("START_OLSR_TC_TASK_INIT\n");
    if(xTaskCreate(olsr_tc_task, "OLSR_TC", 5*configMINIMAL_STACK_SIZE, NULL,
                    LPS_DECK_TASK_PRI, NULL)==pdPASS){
                        DEBUG_PRINT_OLSR_SYSTEM("TC TASK CREATE SUCCESSFUL\n");
                    }else{
                        DEBUG_PRINT_OLSR_SYSTEM("TC TASK CREATE FAILD\n");
                    };
}
static void OLSR_TS_TASK_INIT(){
    DEBUG_PRINT_OLSR_SYSTEM("START_OLSR_TS_TASK_INIT\n");
    if(xTaskCreate(olsr_ts_task, "OLSR_TS", configMINIMAL_STACK_SIZE, NULL,
                    LPS_DECK_TASK_PRI, NULL)==pdPASS){
                        DEBUG_PRINT_OLSR_SYSTEM("TS TASK CREATE SUCCESSFUL\n");
                    }else{
                        DEBUG_PRINT_OLSR_SYSTEM("TS TASK CREATE FAILD\n");
                    };
}
static void OLSR_SEND_TASK_INIT(dwDevice_t *dev){
    DEBUG_PRINT_OLSR_SYSTEM("START_OLSR_SEND_TASK_INIT\n");
    if(xTaskCreate(olsr_send_task, "OLSR_SEND", 5*configMINIMAL_STACK_SIZE, dev,
                    LPS_DECK_TASK_PRI, NULL)==pdPASS){
                        DEBUG_PRINT_OLSR_SYSTEM("SEND TASK CREATE SUCCESSFUL\n");
                    }else{
                        DEBUG_PRINT_OLSR_SYSTEM("SEND TASK CREATE FAILD\n");
                    };
}
static void OLSR_RECV_TASK_INIT(dwDevice_t *dev){
    DEBUG_PRINT_OLSR_SYSTEM("START_OLSR_RECV_TASK_INIT\n");
    if(xTaskCreate(olsr_recv_task, "OLSR_RECV", 5*configMINIMAL_STACK_SIZE, dev,
                    LPS_DECK_TASK_PRI, NULL)==pdPASS){
                        DEBUG_PRINT_OLSR_SYSTEM("RECV TASK CREATE SUCCESSFUL\n");
                    }else{
                        DEBUG_PRINT_OLSR_SYSTEM("RECV TASK CREATE FAILD\n");
                    };
}
static void OLSR_TASK_INIT(dwDevice_t *dev){
    DEBUG_PRINT_OLSR_SYSTEM("TASK_INIT");
    OLSR_HELLO_TASK_INIT();
    OLSR_TC_TASK_INIT();
    OLSR_TS_TASK_INIT();
    OLSR_SEND_TASK_INIT(dev);
    OLSR_RECV_TASK_INIT(dev);
}

static void Initialize(dwDevice_t *dev) {
    systemWaitStart();
    int myChanel = configblockGetRadioChannel();
    myAddress = myChanel|0x0000;
    DEBUG_PRINT_OLSR_SYSTEM("init to new deck OLSR");
    OLSR_STRUCT_INIT(dev);
    OLSR_TASK_INIT(dev);
}

static bool isRangingOk()
{
  return true;
}

point_t anchorPosition[2];
static bool getAnchorPosition(const uint8_t anchorId, point_t* position) {
    *position = anchorPosition[anchorId];
    return true;
}

static uint8_t getAnchorIdList(uint8_t unorderedAnchorList[], const int maxListSize) {
  return 2;
}

static uint8_t getActiveAnchorIdList(uint8_t unorderedAnchorList[], const int maxListSize) {
  return 2;
}

uwbAlgorithm_t uwbOLSRAlgorithm = {
  .init = Initialize,
  .isRangingOk = isRangingOk,
  .getAnchorPosition = getAnchorPosition,
  .getAnchorIdList = getAnchorIdList,
  .getActiveAnchorIdList = getActiveAnchorIdList,
};


