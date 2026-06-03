
#ifndef MODULECOM_H
#define MODULECOM_H


/****************Module IDs************************/
// Module IDs are used to uniquely identify a module while sending messages between modules
#define UNKNOWN			0
#define STARTUP			1
#define CLI_MOD			2
#define CSM_MOD			3			//Compost State machine module
#define TCS_MOD			4			//Transfer control system module
#define TIMER_MOD		5
#define LPUART_ISR		6
#define LEVELSENSE_MOD	7
#define CT_MOD			8			//Compost Turner module
#define HATCS_MOD		9			//Humidity-Aeration-Temperature Control Module
#define SPCS_MOD		10			//Water sprayer module
#define HEATERCS_MOD	11			//Heater control module
#define HELLO_TASK		12
#define SENSOR_MOD		13
#define SHREDDER_MOD	14
#define HMICMDPROC_MOD	15			//HMI command Processor Module
#define LID_MOD			16			// Lid Control Module
#define LIMITSWITCH_MOD 17			// Limit Switch Sensing Module
#define SYSSTART_MOD	18
#define DRV89XX_FH_MOD	19
#define ADCS_MOD		20
#define ALARMMGR_MOD	21
#define LAST_MODULE		22





/***************Command code ************************/
// defines the command carried in the event queue between modules

#define USRCMD_RCVD					0x00

//CSM related commands
#define CSM_WASTE_ADD_START			0x01
#define CSM_WASTE_ADDED				0x02
#define CSM_NOTIFY_TRANSFER_END		0x03
#define CSM_GET_STATE				0x04
#define CSM_START					0x05
#define CSM_STOP					0x06
#define CSM_PAUSE					0x07
#define CSM_RESUME					0x08

//Common Commands
#define CLI_CMD						0x09
#define DG_MODULE_START				0x0A
#define DG_MODULE_STOP				0x0B
#define DG_TIMER_EXPIRY				0x0C


//CT (Compost Turner) module commands
#define CT_CONFIGURE				0x10
#define CT_START					0x11
#define CT_STOP						0x12
#define CT_ONECYCLE					0x13


//Temperature & Humidity Control System commands/events
#define HATCS_SET_PARAM						0x18
#define HATCS_START							0x19
#define HATCS_STOP							0x1A
#define HATCS_NOTIFY_SENSOR_STATE_CHANGE	0x1B
#define HATCS_INSTRUCT_AIRINLET				0x1C
#define HATCS_INSTRUCT_MIXING				0x1D
#define HTACS_LS_MKVALVERECIRC				0x1E
#define HATCS_MKV_SYNC						0x1F

//Water sprayer control commands
#define SPCS_SPRAY_ONCE				0x20

//ADCS Module commands
#define ADCS_START					0x24
#define ADCS_ABORT					0x25


//Transfer Control System
#define TCS_START					0x28
#define TCS_ABORT					0x29
#define DG_LS_STVALVECLOSE			0x2A
#define TCS_STV_SYNC				0x2B

//Sensor Control Module Commands
#define SENSOR_START				0x2C
#define SENSOR_STOP					0x2D
#define SENSOR_SET_PARAM			0x2E
#define GET_SENSOR_STATUS			0x2F


//HMI Command Processor Module API commands
#define HMICMD_BUFFER_RECEIVED		0x30
#define HMICMD_RESPONSE_SENT		0x31
#define GEN_HMI_INTERRUPT			0x32


//LID module commands
#define PROXIMITY_EVENT				0x38
#define LIDSWITCH_EVENT				0x39

//Shredder Module Commands
#define DG_LID_OPEN					0x3C
#define DG_LID_CLOSE				0x3D
#define DG_LS_FLAPCLOSE				0x3E
#define SHD_FLAP_SYNC				0x3F

//Alarm Manager Commands
#define ALARMMGR_RAISE_ALARM        0x40
#define ALARMMGR_CLEAR_ALARM        0x41
#define ALARMMGR_GET_ACTIVE_LIST    0x42
#define ALARMMGR_GET_DEFINITION     0x43
#define ALARMMGR_GET_HISTORY        0x44

/****************** Queue Message Definition ************************/

//This structure will be used to pass messages between modules over Queue.

typedef struct qMessage
{
	uint8_t	src_module;             //Module that sends the message
	uint8_t dest_module;			//Module that receives the message
	uint8_t command;				//Command code indicating the action to be performed by the receiving model
	uint8_t *result;				//result of the action performed returned in this pointer variable
	TaskHandle_t taskHandleSM;		//Handle to the sending task for notification
	void *cmdParam;					//Command parameter. Interpretation is left to src and destination module
} dgMsg_t;


/***********Module Structure************************/
//This structure stores the module details necessary for the communication

typedef struct moduledef
{
	QueueHandle_t modQHandle;
	TaskHandle_t  modTaskhandle;
	TimerHandle_t modTimerHandle;
}dgModule_t;




/********** Prototypes ***********************************/

int	registerModule(uint8_t moduleId, TaskHandle_t modTaskHandle, TimerHandle_t timerHandle );


int sendMsg(dgMsg_t *message);
int sendMsgFromISR(dgMsg_t *message);

QueueHandle_t getQHandle(uint8_t moduleid);

TaskHandle_t getTaskHandle(uint8_t moduleid);

TimerHandle_t getTimerHandle(uint8_t moduleid);

int moduleStart(uint8_t moduleId);

int moduleStop(uint8_t moduleId);

void initModuleStore(void);

#endif /* MODULECOM_H */
