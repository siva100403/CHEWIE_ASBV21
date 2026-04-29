/*
 * cliProc.h
 *
 *  Created on: 23-Nov-2024
 *      Author: Jawahar Arumugam
 */

#ifndef CLIPROC_H_
#define CLIPROC_H_



//Command structure definition

#define CMD_MAX_SIZE    256
#define CLI_RESP_MAX_SIZE	256

//Commands will be received from multiple channels (such as Serial, Local Mobile, remote server, etc )
//Command processor will process only one command at a time. Hence each channel should send only one command
// at a time. However, all channels can send a command simultaneously. Command Processor will keep the command
// in queue and process in the order of reception

// Each channel should create a command buffer and populate the state & command and send a NEW_COMMAND event to the
//command processor. When the command is completed command processor will send CMD_COMPLETE event to the respective
//Channel
struct cmd_buffer
{
	uint8_t cmd_state;
	uint8_t cmd_channel;
	char command[CMD_MAX_SIZE];
};

//Valid states for ""cmd_state" in "cmd_buffer" structure
#define CMD_IDLE		0       //On power up it will be initialized to IDLE state
#define CMD_RECEIVING	1		//Either waiting for command to arrive or in the middle of receiving
#define CMD_RECEIVED	2		//Command received and sent to command processor
#define CMD_INPROCESS	3		//Command taken up by Command Processor for execution
#define CMD_COMPLETED	4		// Command execution completed and send message to the Command channel
								// After sending response back to the user the state will be either changed
								// to IDLE or receiving depending on the channel



//Supported commands and their command codes

#define HEATER			1
#define SHD_MOTOR		2
#define DC_SPRAYER		3
#define FLUSH_SPRAYER	4
#define ADDITIVE_DISPR	5
#define ST_MOTOR		6
#define AIR_VALVE1		7
#define AIR_VALVE2		8
#define AIR_VALVE3		9
#define FAN_MOTOR		10
#define AUGER_MOTOR		11
#define SHDAUG_MOTOR      	12
#define SHDAUG_MOTOR_SPEED 	13
#define FLAP_MOTOR     	14

#define GETTIME			15
#define SETTIME			16
#define GETTEMP			17
#define GETSENSORSTATUS	18
#define GETCSMSTATUS	19
#define ADD_WASTE_START	20
#define ADD_WASTE_END	21
#define CTCS			22
#define LID				23
#define TCS				24
#define ADCS			25
#define SHDCS			26
#define CSMSTART		27
#define CSMSTOP			28
#define GETCSMSTATUS_CHTL		29
#define GETSENSORSTATUS_CHTL	30
#define GETAUGERCFG_C	31
#define SETAUGERCFG_C	32
#define GETHATCSCFG_C	33
#define SETHATCSCFG_C	34
#define GETCSMCFG_C		35
#define SETCSMCFG_C		36
#define GETTRFRCFG_C	37
#define SETTRFRCFG_C	38
#define GETSHDSEQ_C		39
#define SETSHDSEQ_C		40
#define GETSHDTVAR_C	41
#define SETSHDTVAR_C	42
#define SAVECONFIGEE_C	43
#define FD_CONFIG		44
#define MOTOR1			45
#define MOTOR2			46
#define SPARE2_RELAY	47
#define GETLS_STATUS	48
#define MKVALVE			49
#define GETSYSCFGVER_C	50
#define GETADCSCFG_C	51
#define SETADCSCFG_C	52





/*******************************************************************************
 * Function prototypes
 ******************************************************************************/

int initCli(void);
int sendCliCmd(uint8_t srcModule, char* cmdstring);
void sendCliResponse(char* response, uint8_t size);

#endif /* CLIPROC_H_ */
