/*
 * alarmManager.h
 *
 *  Created on: 01-Apr-2026
 */

#ifndef ALARMMANAGER_H_
#define ALARMMANAGER_H_

#include <stdint.h>
#include "rtc.h"

/**************** Alarm Manager Constants ****************/
#define ALARMMGR_MAX_ACTIVE_ALARMS      16U
#define ALARMMGR_MAX_HISTORY_ENTRIES    64U
#define ALARMMGR_MAX_COMMENT_LEN        48U
#define ALARMMGR_MAX_DESC_LEN           96U

/* Alarm Manager module states */
#define ALARMMGR_STATE_IDLE             0U
#define ALARMMGR_STATE_READY            1U

/* Alarm Groups */
typedef enum
{
    ALARM_GROUP_HWFAULT = 1,
    ALARM_GROUP_OPERATION = 2,
    ALARM_GROUP_CONFIG = 3,
    ALARM_GROUP_VER_MISMATCH = 4
} dgAlarmGroup_t;

/* Alarm Categories */
typedef enum
{
    ALARM_CATEGORY_CRITICAL = 1,
    ALARM_CATEGORY_MAJOR = 2,
    ALARM_CATEGORY_MINOR = 3,
    ALARM_CATEGORY_INFO = 4
} dgAlarmCategory_t;

/* Alarm history event type */
typedef enum
{
    ALARM_HISTORY_RAISED = 1,
    ALARM_HISTORY_CLEARED = 2
} dgAlarmHistoryEvent_t;



typedef struct
{
    uint16_t alarmId;
    dgAlarmGroup_t group;
    dgAlarmCategory_t category;
    uint8_t reportToCloud;
    uint8_t showOnHmi;
    uint8_t affectsOperation;
    char description[ALARMMGR_MAX_DESC_LEN];
} dgAlarmDefinition_t;

typedef struct
{
    uint16_t alarmId;
    dgDateTime_t raisedTime;
    char comment[ALARMMGR_MAX_COMMENT_LEN];
} dgActiveAlarm_t;

typedef struct
{
    uint16_t alarmId;
    uint8_t status; /* dgAlarmHistoryEvent_t */
    dgDateTime_t eventTime;
    char comment[ALARMMGR_MAX_COMMENT_LEN];
} dgAlarmHistory_t;

typedef struct
{
    uint16_t alarmId;
    const char *comment;
} dgAlarmRaiseReq_t;

typedef struct
{
    uint16_t alarmId;
} dgAlarmClearReq_t;

typedef struct
{
    dgActiveAlarm_t *alarmList;
    uint16_t maxAlarmCount;
    uint16_t returnedAlarmCount;
} dgAlarmGetActiveListReq_t;

typedef struct
{
    dgAlarmDefinition_t *alarmDefList;
    uint16_t maxAlarmCount;
    uint16_t returnedAlarmCount;
} dgAlarmGetDefinitionReq_t;

typedef struct
{
    dgAlarmHistory_t *historyList;
    uint16_t maxHistoryCount;
    uint16_t returnedHistoryCount;
} dgAlarmGetHistoryReq_t;

int initAlarmManager(void);

#endif /* ALARMMANAGER_H_ */
