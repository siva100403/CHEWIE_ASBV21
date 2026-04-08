/*
 * sysConfig.h
 *
 *  Created on: 22-Aug-2025
 *      Author: Jawahar Arumugam
 */

#ifndef SYSCONFIG_H_
#define SYSCONFIG_H_


/*******************************************************************************
 * Macro definitions and Constants
 ******************************************************************************/

/*------------------------ CSM Constants -------------------------------------*/
//Window used to correct Temperature and Humidity
#define TEMP_CORRECTION_ERROR		2.0
#define HUMIDITY_CORRECTION_ERROR	5.0


//Composting phase definitions
#define MESOPHILIC_PHASE	0
#define THERMOPHILIC_PHASE	1
#define PATHOGEN_ELM_PHASE	2
#define TRANSFER_PHASE		3
#define PHASE_IDLE			4
//#define PHASE_MAX			5

#define PHASE_COUNT_MAX		3
//Input wet-waste composition categories
//It is envisaged that the composting process parameters have to be modified based on the
// composition of the wet-waste.
//To simplify the logic, wet-waste will be grouped under 5 category and process parameters
// can be configured differently for these category.
//Categorizing the input  wet-watse will be done by the AI module using camera frames

#define WASTE_CAT0		0
#define WASTE_CAT1		1
#define WASTE_CAT2		2
#define WASTE_CAT3		3
#define WASTE_CAT4		4
#define WASTE_CAT5		5
#define WASTE_CAT6		6
#define WASTE_CAT7		7
#define WASTE_CAT_MAX	8

#define CSM_DEFAULT_WASTE_CAT  WASTE_CAT0

/*------------------------ HATCS module Constants -----------------------------*/
#define MAX_CTRL_CODE_PER_SEQ			8



/*******************************************************************************
 * Data structures
 ******************************************************************************/


typedef struct chewieAbout
{
	char sysName[16];
	char model[16];
	char asbver[8];
	char aibver[8];
	char hmiver[8];
	int serialNo;
	int dummy;

}dgChewieAbout_t;

/*-------------- Data structures used by CT CS Module ------------------------*/


typedef struct ctConfigParam
{
	uint8_t		cwrDuration;           			// in secs; clockwise rotation duration
	uint8_t		ccwrDuration;					// in secs; Counter clockwise rotation duration
	uint8_t 	pauseDuration;					// in secs; Pause duration while changing direction
	uint8_t 	rptCount;						// Repeat count for one auger cycle
	uint8_t		offDuration;					// in secs; Off duration between cycles

} dgCtConfigParam_t;


/*-------------- Data structures used by CSM  Module ------------------------*/

//This structure defines the process parameters used by the compost chamber.
//The compost chamber may use different set of parameters based on the
//composting phase and the composition of wet waste input
typedef struct ctProcessParam
{
	float		temperature;		//Composting Chamber desired Temperature in Deg C
	float		humidity;			//Composting Chamber desired humidity; Value 0 to 100
	uint32_t	dummy;				//Dummy to get EEPROM Page alignment
	uint16_t	phaseDur; 			//Indicates in min how long to remain in a phase
	uint16_t	aeration;			//Indicates in min the aeration interval
}dgCtProcessParam_t;

typedef struct ctPhaseProcessParam
{
	dgCtProcessParam_t mPhase;
	dgCtProcessParam_t tPhase;
	dgCtProcessParam_t pPhase;
	dgCtProcessParam_t dPhase;				//For 64 byte alignment

}dgCtPhaseProcessParam_t;


//This array defines an array to store the process parameters for each composting phase and
//input wet-waste composition
//dgCtPhaseProcessParam_t processParamTable[WASTE_CAT_MAX];

typedef struct ctWastecatProcessParam
{
	dgCtPhaseProcessParam_t	  cat0;
	dgCtPhaseProcessParam_t	  cat1;
	dgCtPhaseProcessParam_t	  cat2;
	dgCtPhaseProcessParam_t	  cat3;
	dgCtPhaseProcessParam_t	  cat4;
	dgCtPhaseProcessParam_t	  cat5;
	dgCtPhaseProcessParam_t	  cat6;
	dgCtPhaseProcessParam_t	  cat7;

}dgCtWastecatProcessParam_t;

/*-------------- Data structures used by HATCS  Module ------------------------*/

//This structure defines the control code for different actuators

typedef struct actuatorCtrlCode
{
	uint8_t		duration;			//in min; How long the control code should be active
	uint8_t		ctCtrl;				//ct Control code (CT_ON/CT_OFF/CT_ONCE)
	uint8_t		heaterCtrl;			//Heater Control code (HTR_ON/HTR_OFF)
	uint8_t		sprayerCtrl; 		//Sprayer Control Code (SPRAYER_ONCE/SPRAYER_OFF)
	uint8_t		fanCtrl;			//Fan Control code (FAN_OFF/FAN_CWR/FAN_CCWR)
	uint8_t 	ctrlSeqEnd;			//Indicates the end of control sequence (END/START/MID)
	uint8_t 	airCircCtrl;		//Air Circulation control (AIR_IN, AIR_OUT, AIR_RECIRC)
	uint8_t 	dummy;				//For 8 byte boundary
}dgActuatorCtrlCode_t;

//This structure defines the actuation sequence of actuators for a HAT state
//There are 9 HAT states and hence there will be 9 elements in the structure
//Each structure element is an array of 8 actuator control codes
//These control codes will be executed as per the order specified for the duration specified.
//Number of control codes in a sequence can be different and the last control code in the seq
//will have the "ctrlSeqEnd" in dgActuatorCtrlCode_t structure will have "END" to indicate the
//end of Actuator Control code sequence.


typedef struct actuatorCtrlSeq
{
	dgActuatorCtrlCode_t  temp_wr_hum_wr[MAX_CTRL_CODE_PER_SEQ];		//Ctrl sequence when the temp and humidity within range
	dgActuatorCtrlCode_t  temp_wr_hum_ar[MAX_CTRL_CODE_PER_SEQ];		//Ctrl sequence when the temp within range and humidity above range
	dgActuatorCtrlCode_t  temp_wr_hum_br[MAX_CTRL_CODE_PER_SEQ];		//Ctrl sequence when the temp within range and humidity below range
	dgActuatorCtrlCode_t  temp_ar_hum_wr[MAX_CTRL_CODE_PER_SEQ];		//Ctrl sequence when the temp above range and humidity within range
	dgActuatorCtrlCode_t  temp_ar_hum_ar[MAX_CTRL_CODE_PER_SEQ];		//Ctrl sequence when the temp above range and humidity above range
	dgActuatorCtrlCode_t  temp_ar_hum_br[MAX_CTRL_CODE_PER_SEQ];		//Ctrl sequence when the temp above range and humidity below range
	dgActuatorCtrlCode_t  temp_br_hum_wr[MAX_CTRL_CODE_PER_SEQ];		//Ctrl sequence when the temp below range and humidity within range
	dgActuatorCtrlCode_t  temp_br_hum_ar[MAX_CTRL_CODE_PER_SEQ];		//Ctrl sequence when the temp below range and humidity above range
	dgActuatorCtrlCode_t  temp_br_hum_br[MAX_CTRL_CODE_PER_SEQ];		//Ctrl sequence when the temp below range and humidity below range
}dgActuatorCtrlSeq_t;


/*typedef struct transferCtrlSeq
{
	uint16_t duration; 			//in 100s of mSec
	uint8_t ctCtrl;				// TRSEQ_CTMOTOR_CCW, TRSEQ_CTMOTOR_OFF
	uint8_t stCtrl;				// TRSEQ_STMOTOR_ON, TRSEQ_STMOTOR_OFF
	uint8_t ctrlSeqRecType;		// TRSEQ_CTRL_START, TRSEQ_CTRL_MID, TRSEQ_CTRL_END

}dgTransferCtrlSeq_t;*/

typedef struct tcsConfig
{
	uint16_t stvOpenDur;		//in mSec
	uint16_t stvCloseDur;		//in mSec
	uint16_t transferDur;		//in Sec

}dgTcsConfigParam_t;


/***********************Shredder CS Configuration Parameters ******************************/

#define MAX_SHD_SEQ		16
typedef struct shredderCtrlSeq
{
	uint16_t durationSec; 			//in seconds
	uint8_t shdMotor;
	uint8_t shdFlapMotor;
	uint8_t flushSprayer;
	uint8_t shdAugMotor;
	uint8_t ctrlSeqRecType;
}dgShredderCtrlSeq_t;


typedef struct shredderTimingVar
{
	uint8_t shdStartDlyFmLidclose;		//in sec. Decided based on water drain time
	uint8_t flapOpenDur;				//in sec. The duration flap motor has to be ON for Opening the flap
	uint8_t flapCloseDur;				//in sec. The duration flap motor has to be ON for closing the flap
	uint8_t numOfCtrlSeq;				// Number of actuator control sequences in

}dgShredderTimingVar_t;

typedef struct shdConfigParams
{
	dgShredderTimingVar_t	shdTimingVars;
	dgShredderCtrlSeq_t shdActCtrlSeq[MAX_SHD_SEQ-1];
}dgShdConfigParams_t;


/*-----------------------------------------------------------------------------------------*/

//16 pages (128 byte each) ate allocated in EEPROM for storing Configuration parameters

typedef union  hatcsCfg  //Page 0 to 5
{
	uint8_t 				hatcsSpace[6*128];
	dgActuatorCtrlSeq_t		hatcsActuatorCtrlSeq;
}dgHatcsConfig_t;
typedef union csmCfg
{
	uint8_t 				csmSpace[4*128];
	dgCtWastecatProcessParam_t	csmProcessParam;

}dgCsmConfig_t;

typedef union augerCfg
{
	uint8_t				augerSpace[128];
	dgCtConfigParam_t	augerParam;
}dgAugerConfig_t;

typedef union transferCfg
{
	uint8_t 			transferSpcae[128];
	dgTcsConfigParam_t	transferParams;
}dgTransferConfig_t;


typedef union shredderCfg
{
	uint8_t 			transferSpcae[128];
	dgShdConfigParams_t	shdParams;
}dgShredderConfig_t;
typedef struct configMemAllocation
{
	dgHatcsConfig_t hatcsConfig;		//Page 0 to 5
	dgCsmConfig_t csmConfig;			//Page 6 to 9
	dgAugerConfig_t augerConfig;		//Page 10
	dgTransferConfig_t transferConfig;	//Page 11
	dgShredderConfig_t shredderConfig;	//Page 12
	uint8_t additiveCfg[128];			//Page 13
	uint8_t dummy[128];					//Page 14
	uint8_t checksumArea[128];  		//Page 15
}dgConfigMem_t;
/*******************************************************************************
 * Function prototypes
 ******************************************************************************/

//int	loadCtParam(dgCtConfigParam_t *ctParamCur);
int	loadProcessParam(dgCtProcessParam_t *processParam, uint8_t wasteCat, uint8_t phase);
int	loadActuatorSeq(dgActuatorCtrlCode_t *controlCode, uint8_t hatSensorState);

int initSysConfig();

// Shredder
int getShredderCtrlSeq(uint8_t index, dgShredderCtrlSeq_t *out);
int setShredderCtrlSeq(uint8_t index, const dgShredderCtrlSeq_t *value);
int getShredderTimingParam(dgShredderTimingVar_t *out);
int setShredderTimingParam(const dgShredderTimingVar_t *value);

// Transfer
int setTransferCtrlParams(const dgTcsConfigParam_t *value);
int getTransferCtrlParams(dgTcsConfigParam_t *out);

// Auger
int getAugerConfig(dgCtConfigParam_t *out);
int setAugerConfig(const dgCtConfigParam_t *value);

// HATCS Actuator Control Sequence
int getHatcsActuatorCtrlSeq(uint8_t sensorState, uint8_t index, dgActuatorCtrlCode_t *out);
int setHatcsActuatorCtrlSeq(uint8_t sensorState, uint8_t index, const dgActuatorCtrlCode_t *value);

// CSM Process Parameters
int getCsmPhaseParam(uint8_t wasteCat, uint8_t phase, dgCtProcessParam_t *out);
int setCsmPhaseParam(uint8_t wasteCat, uint8_t phase, dgCtProcessParam_t *value);

int storeAllconfigEEPROM();
int loadDefaultConfig();


#endif /* SYSCONFIG_H_ */
