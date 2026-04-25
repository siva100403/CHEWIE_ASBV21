/*
 * alarmManager.c
 *
 *  Created on: 01-Apr-2026
 */

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dgCommon.h"
#include "modulecom.h"
#include "alarmManager.h"
#include "rtc.h"
#include "shredder.h"
#include "motorControl.h"
#include "cliProc.h"
#include "dgTimer.h"
#include "pin_mux.h"
#include "GPIOSignals.h"

#define ALARMMGR_TASK_STACK_SIZE      1024U

static TaskHandle_t alarmMgrTaskHandle;
static TimerHandle_t alarmMgrTimerHandle;

static dgActiveAlarm_t gActiveAlarmList[ALARMMGR_MAX_ACTIVE_ALARMS];
static uint16_t gActiveAlarmCount = 0U;

static dgAlarmHistory_t gAlarmHistoryList[ALARMMGR_MAX_HISTORY_ENTRIES];
static uint16_t gAlarmHistoryCount = 0U;
static uint16_t gAlarmHistoryWriteIndex = 0U;

static const dgAlarmDefinition_t gAlarmDefinitionList[] =
{
    { 1U, ALARM_GROUP_HWFAULT,      ALARM_CATEGORY_CRITICAL, 1U, 1U, 1U, "HW_ALARM_TEMP_SENSOR_FAILURE" },
    { 2U, ALARM_GROUP_HWFAULT,      ALARM_CATEGORY_MAJOR,    1U, 1U, 1U, "HW_ALARM_HUM_SENSOR_FAILURE" },
    { 3U, ALARM_GROUP_HWFAULT,      ALARM_CATEGORY_MAJOR,    1U, 1U, 1U, "HWS_ALARM_LID_FAULT" },
    { 4U, ALARM_GROUP_OPERATION,    ALARM_CATEGORY_MINOR,    1U, 1U, 0U, "OPS_ALARM_ODOURSHIELD_REPLACE" },
    { 5U, ALARM_GROUP_CONFIG,       ALARM_CATEGORY_MAJOR,    1U, 1U, 1U, "CONFIG_ALARM_INVALID_SETTING" },
    { 6U, ALARM_GROUP_VER_MISMATCH, ALARM_CATEGORY_INFO,     1U, 1U, 0U, "ALARM_GROUP_VER_MISMATCH" }
};

#define ALARMMGR_TOTAL_DEFINITIONS ((uint16_t)(sizeof(gAlarmDefinitionList) / sizeof(gAlarmDefinitionList[0])))

static void alarmManager_task(void *pvParameters);
static int alarmMgrHandleRaiseAlarm(dgAlarmRaiseReq_t *req);
static int alarmMgrHandleClearAlarm(dgAlarmClearReq_t *req);
static int alarmMgrHandleGetActiveList(dgAlarmGetActiveListReq_t *req);
static int alarmMgrHandleGetDefinition(dgAlarmGetDefinitionReq_t *req);
static int alarmMgrHandleGetHistory(dgAlarmGetHistoryReq_t *req);
static int alarmMgrFindDefinition(uint16_t alarmId, const dgAlarmDefinition_t **defPtr);
static int alarmMgrFindActiveIndex(uint16_t alarmId);
static void alarmMgrGetCurrentTime(dgDateTime_t *timestamp);
static void alarmMgrStoreHistory(uint16_t alarmId, uint8_t status, const char *comment);
static void alarmMgrNotifyStakeholders(uint16_t alarmId, uint8_t status);
static void alarmMgrFlushHistoryToNvm(void);
static void alarmMgrSendResponse(dgMsg_t *msg, uint8_t result);



int initAlarmManager(void)
{
    int ret;

    alarmMgrTaskHandle = NULL;
    alarmMgrTimerHandle = NULL;
    gActiveAlarmCount = 0U;
    gAlarmHistoryCount = 0U;
    gAlarmHistoryWriteIndex = 0U;
    memset(gActiveAlarmList, 0, sizeof(gActiveAlarmList));
    memset(gAlarmHistoryList, 0, sizeof(gAlarmHistoryList));

    ret = xTaskCreate(alarmManager_task,
                      "alarmMgr",
                      ALARMMGR_TASK_STACK_SIZE,
                      NULL,
                      task_PRIORITY,
                      &alarmMgrTaskHandle);
    if(ret != pdPASS)
    {
        printf("alarmManager.c:initAlarmManager():xTaskCreate failed\r\n");
        return DG_FAIL;
    }

    //Alarm Manager requires timer and hence create FreeRTOS SW timer
    alarmMgrTimerHandle = xTimerCreate("alarmMgrTimer",2000, pdTRUE, (void*)ALARMMGR_MOD, dgTimerCallback);
    if(alarmMgrTimerHandle == NULL)
    {
        printf("Timer creation failed!.\r\n");
    	vTaskDelete(alarmMgrTaskHandle);
        return DG_FAIL;
    }

    ret = registerModule(ALARMMGR_MOD, alarmMgrTaskHandle, alarmMgrTimerHandle);
    if(ret != DG_SUCCESS)
    {
        printf("alarmManager.c:initAlarmManager():registerModule failed\r\n");
        return ret;
    }
    printf("alarmManager.c:initAlarmManager():passed\r\n");
    return DG_SUCCESS;
}

static void alarmManager_task(void *pvParameters)
{
    QueueHandle_t alarmMgrQHandle;
    dgMsg_t rcvMsg;
    int alarmMgrState;
    int ret;

    (void)pvParameters;

    alarmMgrQHandle = NULL;
    while(alarmMgrQHandle == NULL)
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        alarmMgrQHandle = getQHandle(ALARMMGR_MOD);
    }

    alarmMgrState = ALARMMGR_STATE_IDLE;
    printf("alarmManager.c:alarmManager_task():started\r\n");

    while(1)
    {
        if(xQueueReceive(alarmMgrQHandle, &rcvMsg, portMAX_DELAY) != pdPASS)
        {
            continue;
        }
        printf("alarmManager.c:alarmManager_task():Cmd rcvd= %d\r\n", rcvMsg.command);
        ret = DG_SUCCESS;

        switch(alarmMgrState)
        {
        case ALARMMGR_STATE_IDLE:
            switch(rcvMsg.command)
            {
            case DG_MODULE_START:
                alarmMgrState = ALARMMGR_STATE_READY;
                //start timer for alarm scanning
				//Start 2 minute timer
				dgtimerStart(ALARMMGR_MOD, CONV_SEC_TO_TICKS(120));

                break;

            case DG_MODULE_STOP:
                alarmMgrState = ALARMMGR_STATE_IDLE;
				dgtimerStop(ALARMMGR_MOD);
                break;

            case DG_TIMER_EXPIRY:
            	//Timer should not be active. Hence stop
				dgtimerStop(ALARMMGR_MOD);
                break;

            case ALARMMGR_RAISE_ALARM:
            case ALARMMGR_CLEAR_ALARM:
            case ALARMMGR_GET_ACTIVE_LIST:
            case ALARMMGR_GET_DEFINITION:
            case ALARMMGR_GET_HISTORY:
                ret = DG_INVALID_STATE;
                alarmMgrSendResponse(&rcvMsg, (uint8_t)ret);
                break;

            default:
                printf("alarmManager.c:alarmManager_task():Invalid command %d\r\n", rcvMsg.command);
                ret = DG_INVALID_CMD;
                alarmMgrSendResponse(&rcvMsg, (uint8_t)ret);
                break;
            }
        	break;
        case ALARMMGR_STATE_READY:
            switch(rcvMsg.command)
            {
            case DG_MODULE_START:
                alarmMgrState = ALARMMGR_STATE_READY;
                ret = DG_SUCCESS;
                alarmMgrSendResponse(&rcvMsg, (uint8_t)ret);
                break;

            case DG_MODULE_STOP:
                alarmMgrState = ALARMMGR_STATE_IDLE;
				dgtimerStop(ALARMMGR_MOD);
                ret = DG_SUCCESS;
                alarmMgrSendResponse(&rcvMsg, (uint8_t)ret);
                break;

            case DG_TIMER_EXPIRY:

            	//Scan alarms. Currently 2A motor error state
                printf("alarmManager.c:Scanning for Alarms\r\n");
                if((READ_TC78H660_ERR_STATUS() & 0x01) == 0)
                {
                	dgAlarmRaiseReq_t request;
                	request.alarmId = HW_ALARM_TC78H660;
                	request.comment = "TC78H660 Error Flag set\r\n";
                    printf("alarmManager.c:alarmManager_task():TC78H660 Error Flag active\r\n");
					sendCliResponse("2A Motor driver Error Flag Active\r\n", 35);
					//Correct the error
					HEATER_OFF();
					//Update the Alarm database with Alarm
					alarmMgrHandleRaiseAlarm(&request);
                }
                break;

            case ALARMMGR_RAISE_ALARM:
                ret = alarmMgrHandleRaiseAlarm((dgAlarmRaiseReq_t *)rcvMsg.cmdParam);
                alarmMgrSendResponse(&rcvMsg, (uint8_t)ret);
                break;

            case ALARMMGR_CLEAR_ALARM:

				ret = alarmMgrHandleClearAlarm((dgAlarmClearReq_t *)rcvMsg.cmdParam);
                alarmMgrSendResponse(&rcvMsg, (uint8_t)ret);
                break;

            case ALARMMGR_GET_ACTIVE_LIST:
                ret = alarmMgrHandleGetActiveList((dgAlarmGetActiveListReq_t *)rcvMsg.cmdParam);
                alarmMgrSendResponse(&rcvMsg, (uint8_t)ret);
                break;

            case ALARMMGR_GET_DEFINITION:
                ret = alarmMgrHandleGetDefinition((dgAlarmGetDefinitionReq_t *)rcvMsg.cmdParam);
                alarmMgrSendResponse(&rcvMsg, (uint8_t)ret);
                break;

            case ALARMMGR_GET_HISTORY:
                ret = alarmMgrHandleGetHistory((dgAlarmGetHistoryReq_t *)rcvMsg.cmdParam);
                alarmMgrSendResponse(&rcvMsg, (uint8_t)ret);
                break;

            default:
                printf("alarmManager.c:alarmManager_task():Invalid command %d\r\n", rcvMsg.command);
                ret = DG_INVALID_CMD;
                alarmMgrSendResponse(&rcvMsg, (uint8_t)ret);
                break;
            }
        	break;
        }

    }
}

static int alarmMgrHandleRaiseAlarm(dgAlarmRaiseReq_t *req)
{
    const dgAlarmDefinition_t *defPtr;
    int index;

    if(req == NULL)
    {
        return DG_INVALID_PARAM;
    }

    if(alarmMgrFindDefinition(req->alarmId, &defPtr) != DG_SUCCESS)
    {
        return DG_INVALID_PARAM;
    }

    index = alarmMgrFindActiveIndex(req->alarmId);
    if(index >= 0)
    {
        return DG_BUSY;
    }

    if(gActiveAlarmCount >= ALARMMGR_MAX_ACTIVE_ALARMS)
    {
        return DG_BUSY;
    }

    gActiveAlarmList[gActiveAlarmCount].alarmId = req->alarmId;
    alarmMgrGetCurrentTime(&gActiveAlarmList[gActiveAlarmCount].raisedTime);
    memset(gActiveAlarmList[gActiveAlarmCount].comment, 0, ALARMMGR_MAX_COMMENT_LEN);
    if(req->comment != NULL)
    {
        strncpy(gActiveAlarmList[gActiveAlarmCount].comment, req->comment, ALARMMGR_MAX_COMMENT_LEN - 1U);
    }
    gActiveAlarmCount++;

    alarmMgrStoreHistory(req->alarmId, ALARM_HISTORY_RAISED, req->comment);
    alarmMgrNotifyStakeholders(req->alarmId, ALARM_HISTORY_RAISED);
    (void)defPtr;

    return DG_SUCCESS;
}

static int alarmMgrHandleClearAlarm(dgAlarmClearReq_t *req)
{
    int index;

    if(req == NULL)
    {
        return DG_INVALID_PARAM;
    }

    index = alarmMgrFindActiveIndex(req->alarmId);
    if(index < 0)
    {
        return DG_INVALID_STATE;
    }

    alarmMgrStoreHistory(req->alarmId, ALARM_HISTORY_CLEARED, gActiveAlarmList[index].comment);
    alarmMgrNotifyStakeholders(req->alarmId, ALARM_HISTORY_CLEARED);

    if((uint16_t)index < (gActiveAlarmCount - 1U))
    {
        memmove(&gActiveAlarmList[index],
                &gActiveAlarmList[index + 1],
                (size_t)(gActiveAlarmCount - (uint16_t)index - 1U) * sizeof(dgActiveAlarm_t));
    }
    memset(&gActiveAlarmList[gActiveAlarmCount - 1U], 0, sizeof(dgActiveAlarm_t));
    gActiveAlarmCount--;

    return DG_SUCCESS;
}

static int alarmMgrHandleGetActiveList(dgAlarmGetActiveListReq_t *req)
{
    uint16_t copyCount;

    if((req == NULL) || (req->alarmList == NULL) || (req->maxAlarmCount == 0U))
    {
        return DG_INVALID_PARAM;
    }

    copyCount = gActiveAlarmCount;
    if(copyCount > req->maxAlarmCount)
    {
        copyCount = req->maxAlarmCount;
    }

    if(copyCount > 0U)
    {
        memcpy(req->alarmList, gActiveAlarmList, (size_t)copyCount * sizeof(dgActiveAlarm_t));
    }
    req->returnedAlarmCount = copyCount;

    if(gActiveAlarmCount > req->maxAlarmCount)
    {
        return DG_BUSY;
    }
    return DG_SUCCESS;
}

static int alarmMgrHandleGetDefinition(dgAlarmGetDefinitionReq_t *req)
{
    uint16_t copyCount;

    if((req == NULL) || (req->alarmDefList == NULL) || (req->maxAlarmCount == 0U))
    {
        return DG_INVALID_PARAM;
    }

    copyCount = ALARMMGR_TOTAL_DEFINITIONS;
    if(copyCount > req->maxAlarmCount)
    {
        copyCount = req->maxAlarmCount;
    }

    memcpy(req->alarmDefList, gAlarmDefinitionList, (size_t)copyCount * sizeof(dgAlarmDefinition_t));
    req->returnedAlarmCount = copyCount;

    if(ALARMMGR_TOTAL_DEFINITIONS > req->maxAlarmCount)
    {
        return DG_BUSY;
    }
    return DG_SUCCESS;
}

static int alarmMgrHandleGetHistory(dgAlarmGetHistoryReq_t *req)
{
    uint16_t copyCount;
    uint16_t startIndex;
    uint16_t i;

    if((req == NULL) || (req->historyList == NULL) || (req->maxHistoryCount == 0U))
    {
        return DG_INVALID_PARAM;
    }

    copyCount = gAlarmHistoryCount;
    if(copyCount > req->maxHistoryCount)
    {
        copyCount = req->maxHistoryCount;
    }

    startIndex = 0U;
    if(gAlarmHistoryCount > copyCount)
    {
        startIndex = (uint16_t)((gAlarmHistoryWriteIndex + ALARMMGR_MAX_HISTORY_ENTRIES - copyCount) % ALARMMGR_MAX_HISTORY_ENTRIES);
    }

    for(i = 0U; i < copyCount; i++)
    {
        req->historyList[i] = gAlarmHistoryList[(startIndex + i) % ALARMMGR_MAX_HISTORY_ENTRIES];
    }
    req->returnedHistoryCount = copyCount;

    if(gAlarmHistoryCount > req->maxHistoryCount)
    {
        return DG_BUSY;
    }
    return DG_SUCCESS;
}

static int alarmMgrFindDefinition(uint16_t alarmId, const dgAlarmDefinition_t **defPtr)
{
    uint16_t i;

    if(defPtr == NULL)
    {
        return DG_INVALID_PARAM;
    }

    for(i = 0U; i < ALARMMGR_TOTAL_DEFINITIONS; i++)
    {
        if(gAlarmDefinitionList[i].alarmId == alarmId)
        {
            *defPtr = &gAlarmDefinitionList[i];
            return DG_SUCCESS;
        }
    }
    return DG_INVALID_PARAM;
}

static int alarmMgrFindActiveIndex(uint16_t alarmId)
{
    uint16_t i;

    for(i = 0U; i < gActiveAlarmCount; i++)
    {
        if(gActiveAlarmList[i].alarmId == alarmId)
        {
            return (int)i;
        }
    }
    return -1;
}

static void alarmMgrGetCurrentTime(dgDateTime_t *timestamp)
{
    if(timestamp == NULL)
    {
        return;
    }

    getRTCtimeMMDDHHMM(timestamp);


}

static void alarmMgrStoreHistory(uint16_t alarmId, uint8_t status, const char *comment)
{
    dgAlarmHistory_t *entry;

    entry = &gAlarmHistoryList[gAlarmHistoryWriteIndex];
    memset(entry, 0, sizeof(dgAlarmHistory_t));
    entry->alarmId = alarmId;
    entry->status = status;
    alarmMgrGetCurrentTime(&entry->eventTime);
    if(comment != NULL)
    {
        strncpy(entry->comment, comment, ALARMMGR_MAX_COMMENT_LEN - 1U);
    }

    gAlarmHistoryWriteIndex = (uint16_t)((gAlarmHistoryWriteIndex + 1U) % ALARMMGR_MAX_HISTORY_ENTRIES);
    if(gAlarmHistoryCount < ALARMMGR_MAX_HISTORY_ENTRIES)
    {
        gAlarmHistoryCount++;
    }
}

static void alarmMgrNotifyStakeholders(uint16_t alarmId, uint8_t status)
{
    (void)alarmId;
    (void)status;
    /* TODO: Add notifications to HMI / C-Cloud / System Manager */
}

static void alarmMgrFlushHistoryToNvm(void)
{
    /* TODO: Add NVM history flush integration */
}

static void alarmMgrSendResponse(dgMsg_t *msg, uint8_t result)
{
    if(msg == NULL)
    {
        return;
    }

    if((msg->taskHandleSM != NULL) && (msg->result != NULL))
    {
        *(msg->result) = result;
        xTaskNotify(msg->taskHandleSM, 0, eNoAction);
    }
}
