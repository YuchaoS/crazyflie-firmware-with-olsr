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
#include "uwbOlsr.h"



static void olsrStructInit(dwDevice_t *dev){
    DEBUG_PRINT_OLSR_SYSTEM("START OLSR_STRUCT_INIT\n");
    olsrStructInitAll(dev);
    olsrDeviceInit(dev);
}
static void olsrHelloTaskInit()
{
    DEBUG_PRINT_OLSR_SYSTEM("START_OLSR_HELLO_TASK_INIT\n");
    if(xTaskCreate(olsrHelloTask, "OLSR_HELLO", 5*configMINIMAL_STACK_SIZE, NULL,
                    LPS_DECK_TASK_PRI, NULL)==pdPASS){
                        DEBUG_PRINT_OLSR_SYSTEM("HELLO TASK CREATE SUCCESSFUL\n");
                    }else{
                        DEBUG_PRINT_OLSR_SYSTEM("HELLO TASK CREATE FAILD\n");
                    };
}
static void OLSR_TC_TASK_INIT()
{
    DEBUG_PRINT_OLSR_SYSTEM("START_OLSR_TC_TASK_INIT\n");
    if(xTaskCreate(olsrTcTask, "OLSR_TC", 5*configMINIMAL_STACK_SIZE, NULL,
                    LPS_DECK_TASK_PRI, NULL)==pdPASS){
                        DEBUG_PRINT_OLSR_SYSTEM("TC TASK CREATE SUCCESSFUL\n");
                    }else{
                        DEBUG_PRINT_OLSR_SYSTEM("TC TASK CREATE FAILD\n");
                    };
}
static void OLSR_TS_TASK_INIT()
{
    DEBUG_PRINT_OLSR_SYSTEM("START_OLSR_TS_TASK_INIT\n");
    if(xTaskCreate(olsr_ts_task, "OLSR_TS", configMINIMAL_STACK_SIZE, NULL,
                    LPS_DECK_TASK_PRI, NULL)==pdPASS){
                        DEBUG_PRINT_OLSR_SYSTEM("TS TASK CREATE SUCCESSFUL\n");
                    }else{
                        DEBUG_PRINT_OLSR_SYSTEM("TS TASK CREATE FAILD\n");
                    };
}
static void OLSR_SEND_TASK_INIT(dwDevice_t *dev)
{
    DEBUG_PRINT_OLSR_SYSTEM("START_OLSR_SEND_TASK_INIT\n");
    if(xTaskCreate(olsrSendTask, "OLSR_SEND", 5*configMINIMAL_STACK_SIZE, dev,
                    LPS_DECK_TASK_PRI, NULL)==pdPASS){
                        DEBUG_PRINT_OLSR_SYSTEM("SEND TASK CREATE SUCCESSFUL\n");
                    }else{
                        DEBUG_PRINT_OLSR_SYSTEM("SEND TASK CREATE FAILD\n");
                    };
}
static void OLSR_RECV_TASK_INIT(dwDevice_t *dev)
{
    DEBUG_PRINT_OLSR_SYSTEM("START_OLSR_RECV_TASK_INIT\n");
    if(xTaskCreate(olsrRecvTask, "OLSR_RECV", 5*configMINIMAL_STACK_SIZE, dev,
                    LPS_DECK_TASK_PRI, NULL)==pdPASS){
                        DEBUG_PRINT_OLSR_SYSTEM("RECV TASK CREATE SUCCESSFUL\n");
                    }else{
                        DEBUG_PRINT_OLSR_SYSTEM("RECV TASK CREATE FAILD\n");
                    };
}
static void olsrTaskInit(dwDevice_t *dev)
{
    DEBUG_PRINT_OLSR_SYSTEM("TASK_INIT");
    olsrHelloTaskInit();
    OLSR_TC_TASK_INIT();
    OLSR_TS_TASK_INIT();
    OLSR_SEND_TASK_INIT(dev);
    OLSR_RECV_TASK_INIT(dev);
}

static void olsrInit(dwDevice_t *dev) 
{
    systemWaitStart();
    DEBUG_PRINT_OLSR_SYSTEM("init to new deck OLSR");
    int myChanel = configblockGetRadioChannel();
    myAddress = myChanel|0x0000;
    olsrStructInit(dev);
    olsrTaskInit(dev);
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
  .init = olsrInit,
  .isRangingOk = isRangingOk,
  .getAnchorPosition = getAnchorPosition,
  .getAnchorIdList = getAnchorIdList,
  .getActiveAnchorIdList = getActiveAnchorIdList,
};


