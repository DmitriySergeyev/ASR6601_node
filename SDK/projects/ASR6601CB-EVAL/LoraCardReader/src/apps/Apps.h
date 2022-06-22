#ifndef APPS_H_
#define APPS_H_
#include "CardReader.h"

#define SEND_BUFFER_SIZE	8

typedef enum
{
	eCardReadOk = 0,
	eCardReadErr = 1,
} eCardReadResult;

typedef struct
{
	eCardReadResult ReadResult;
	uint32_t UtcDateTime;
	uint32_t Id;
	Uid CardUid;
} sSendInfo;

extern void SendBufferPut(sSendInfo SendInfo);
extern sSendInfo SendBufferPop( );
extern uint16_t SendBufferGetCount( );
extern void SendBufferClear( );

extern void CardReaderAppStart();
extern void CardReaderAppLoop();

extern void LoRaWanAppStart();
extern void LoRaWanAppLoop();

#endif