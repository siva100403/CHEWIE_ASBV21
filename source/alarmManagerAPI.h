/*
 * alarmManagerAPI.h
 *
 *  Created on: 01-Apr-2026
 */

#ifndef ALARMMANAGERAPI_H_
#define ALARMMANAGERAPI_H_

#include <stdint.h>
#include "alarmManager.h"

int alarmMgrRaiseAlarm(uint16_t alarmId, const char *comment);
int alarmMgrClearAlarm(uint16_t alarmId);
int alarmMgrGetActiveAlarmList(dgActiveAlarm_t *alarmList, uint16_t maxAlarmCount, uint16_t *returnedCount);
int alarmMgrGetAlarmDefinition(dgAlarmDefinition_t *alarmDefList, uint16_t maxAlarmCount, uint16_t *returnedCount);
int alarmMgrGetAlarmHistory(dgAlarmHistory_t *historyList, uint16_t maxHistoryCount, uint16_t *returnedCount);

#endif /* ALARMMANAGERAPI_H_ */
