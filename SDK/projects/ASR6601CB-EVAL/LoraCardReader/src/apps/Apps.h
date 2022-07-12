#ifndef APPS_H_
#define APPS_H_
#include "CardReader.h"
#include "syslog.h"

#define CARD_INFO_BUFFER_SIZE	8

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
} sCardInfo;

typedef struct
{
	bool isInit;
	uint8_t ver;
} sCardReaderInfo;

extern void SendBufferPut(sCardInfo Info);
extern sCardInfo SendBufferPop(bool isNotInc);
extern void SendBufferInc( );
extern uint16_t SendBufferGetCount( );
extern void SendBufferClear( );
extern uint8_t PrepareCardInfoFrame(sCardInfo Info, uint8_t *buff);

extern void CardReaderAppStart();
extern void CardReaderAppLoop();
extern void CardReaderGetInfo(sCardReaderInfo *Info);

extern void LoRaWanAppStart();
extern void LoRaWanAppLoop();

#endif