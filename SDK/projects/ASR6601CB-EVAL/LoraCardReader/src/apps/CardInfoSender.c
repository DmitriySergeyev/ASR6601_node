#include "apps.h"

typedef struct
{
    sCardInfo Buff[CARD_INFO_BUFFER_SIZE];
    uint16_t idxIn;
    uint16_t idxOut;
} sSendBuffer;

static sSendBuffer SendBuffer = 
{
	.idxIn = 0,
	.idxOut = 0
};

void SendBufferPut(sCardInfo Info)
{
		printf("Put packet\r\n");
    SendBuffer.Buff[SendBuffer.idxIn++] = Info;
    if (SendBuffer.idxIn >= CARD_INFO_BUFFER_SIZE) SendBuffer.idxIn = 0;
}

sCardInfo SendBufferPop( )
{
		printf("Pop packet\r\n");
    sCardInfo retval = SendBuffer.Buff[SendBuffer.idxOut++];
    if (SendBuffer.idxOut >= CARD_INFO_BUFFER_SIZE) SendBuffer.idxOut = 0;
    return retval;
}

uint16_t SendBufferGetCount()
{
    uint16_t retval = 0;
    if (SendBuffer.idxIn < SendBuffer.idxOut) retval = CARD_INFO_BUFFER_SIZE + SendBuffer.idxIn - SendBuffer.idxOut;
    else retval = SendBuffer.idxIn - SendBuffer.idxOut;
    return retval;
}

void SendBufferClear()
{
    SendBuffer.idxIn = 0;
    SendBuffer.idxOut = 0;
		memset(SendBuffer.Buff, 0, sizeof(SendBuffer.Buff));
}

uint8_t PrepareCardInfoFrame(sCardInfo Info, uint8_t *buff)
{
		uint8_t pBuff = 0;
		
		buff[pBuff++] = 0x02;
		/* Результат чтения */
		buff[pBuff++] = (uint8_t)Info.ReadResult;
		/* Время чтения в времени устройства */
		buff[pBuff++] = (uint8_t)(Info.UtcDateTime >> 24);
		buff[pBuff++] = (uint8_t)(Info.UtcDateTime >> 16);
		buff[pBuff++] = (uint8_t)(Info.UtcDateTime >> 8);
		buff[pBuff++] = (uint8_t)(Info.UtcDateTime >> 0);
		/* Идентификатор чтения (счетчик) */
		buff[pBuff++] = (uint8_t)(Info.Id >> 24);
		buff[pBuff++] = (uint8_t)(Info.Id >> 16);
		buff[pBuff++] = (uint8_t)(Info.Id >> 8);
		buff[pBuff++] = (uint8_t)(Info.Id >> 0);	
		/* Информация по карте */
		buff[pBuff++] = Info.CardUid.sak;		
		buff[pBuff++] = Info.CardUid.size;	
		for(uint8_t i = 0; i < Info.CardUid.size; i++)
		{
			buff[pBuff++] = Info.CardUid.uidByte[i];
		}
		return pBuff;
}