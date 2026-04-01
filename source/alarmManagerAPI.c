/*
 * alarmManagerAPI.c
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
#include "alarmManagerAPI.h"

static int alarmMgrSendAndWait(uint8_t command, void *cmdParam)
{
    dgMsg_t sendMsgBuf;
    uint8_t result;

    result = DG_FAIL;
    sendMsgBuf.src_module = UNKNOWN;
    sendMsgBuf.command = command;
    sendMsgBuf.dest_module = ALARMMGR_MOD;
    sendMsgBuf.result = &result;
    sendMsgBuf.cmdParam = cmdParam;
    sendMsgBuf.taskHandleSM = xTaskGetCurrentTaskHandle();
    if(sendMsgBuf.taskHandleSM == NULL)
    {
        printf("alarmManagerAPI.c:alarmMgrSendAndWait():Task handle is null\r\n");
        return DG_FAIL;
    }

    if(sendMsg(&sendMsgBuf) != DG_SUCCESS)
    {
        printf("alarmManagerAPI.c:alarmMgrSendAndWait():Message send failed\r\n");
        return DG_FAIL;
    }

    xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
    return result;
}

int alarmMgrRaiseAlarm(uint16_t alarmId, const char *comment)
{
    dgAlarmRaiseReq_t req;

    req.alarmId = alarmId;
    req.comment = comment;
    return alarmMgrSendAndWait(ALARMMGR_RAISE_ALARM, &req);
}

int alarmMgrClearAlarm(uint16_t alarmId)
{
    dgAlarmClearReq_t req;

    req.alarmId = alarmId;
    return alarmMgrSendAndWait(ALARMMGR_CLEAR_ALARM, &req);
}

int alarmMgrGetActiveAlarmList(dgActiveAlarm_t *alarmList, uint16_t maxAlarmCount, uint16_t *returnedCount)
{
    dgAlarmGetActiveListReq_t req;
    int ret;

    if((alarmList == NULL) || (returnedCount == NULL) || (maxAlarmCount == 0U))
    {
        return DG_INVALID_PARAM;
    }

    req.alarmList = alarmList;
    req.maxAlarmCount = maxAlarmCount;
    req.returnedAlarmCount = 0U;

    ret = alarmMgrSendAndWait(ALARMMGR_GET_ACTIVE_LIST, &req);
    if(ret == DG_SUCCESS)
    {
        *returnedCount = req.returnedAlarmCount;
    }
    return ret;
}

int alarmMgrGetAlarmDefinition(dgAlarmDefinition_t *alarmDefList, uint16_t maxAlarmCount, uint16_t *returnedCount)
{
    dgAlarmGetDefinitionReq_t req;
    int ret;

    if((alarmDefList == NULL) || (returnedCount == NULL) || (maxAlarmCount == 0U))
    {
        return DG_INVALID_PARAM;
    }

    req.alarmDefList = alarmDefList;
    req.maxAlarmCount = maxAlarmCount;
    req.returnedAlarmCount = 0U;

    ret = alarmMgrSendAndWait(ALARMMGR_GET_DEFINITION, &req);
    if(ret == DG_SUCCESS)
    {
        *returnedCount = req.returnedAlarmCount;
    }
    return ret;
}

int alarmMgrGetAlarmHistory(dgAlarmHistory_t *historyList, uint16_t maxHistoryCount, uint16_t *returnedCount)
{
    dgAlarmGetHistoryReq_t req;
    int ret;

    if((historyList == NULL) || (returnedCount == NULL) || (maxHistoryCount == 0U))
    {
        return DG_INVALID_PARAM;
    }

    req.historyList = historyList;
    req.maxHistoryCount = maxHistoryCount;
    req.returnedHistoryCount = 0U;

    ret = alarmMgrSendAndWait(ALARMMGR_GET_HISTORY, &req);
    if(ret == DG_SUCCESS)
    {
        *returnedCount = req.returnedHistoryCount;
    }
    return ret;
}
