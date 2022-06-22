#include "apps.h"


typedef struct
{
    sSendInfo Buff[SEND_BUFFER_SIZE];
    uint16_t idxIn;
    uint16_t idxOut;
} sSendBuffer;

static sSendBuffer SendBuffer = 
{
	.idxIn = 0,
	.idxOut = 0
};

void SendBufferPut(sSendInfo SendInfo)
{
		printf("Put packet\r\n");
    SendBuffer.Buff[SendBuffer.idxIn++] = SendInfo;
    if (SendBuffer.idxIn >= SEND_BUFFER_SIZE) SendBuffer.idxIn = 0;
}

sSendInfo SendBufferPop( )
{
		printf("Pop packet\r\n");
    sSendInfo retval = SendBuffer.Buff[SendBuffer.idxOut++];
    if (SendBuffer.idxOut >= SEND_BUFFER_SIZE) SendBuffer.idxOut = 0;
    return retval;
}

uint16_t SendBufferGetCount()
{
    uint16_t retval = 0;
    if (SendBuffer.idxIn < SendBuffer.idxOut) retval = SEND_BUFFER_SIZE + SendBuffer.idxIn - SendBuffer.idxOut;
    else retval = SendBuffer.idxIn - SendBuffer.idxOut;
    return retval;
}

void SendBufferClear()
{
    SendBuffer.idxIn = 0;
    SendBuffer.idxOut = 0;
		memset(SendBuffer.Buff, 0, sizeof(SendBuffer.Buff));
}