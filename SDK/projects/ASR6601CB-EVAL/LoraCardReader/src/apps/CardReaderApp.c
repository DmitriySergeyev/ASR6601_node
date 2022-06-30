#include "apps.h"
#include "HwCardReader.h"
#include "CardReader.h"

static bool isInit = false;
static uint32_t CountNewCard = 0;

static bool ShowReaderDetails() 
{
	// Get the MFRC522 software version
	uint8_t v = PCD_ReadRegister(VersionReg);
	printf("MFRC522 Software Version: 0x0%2X", v);
	if (v == 0x91)
	{
		printf(" = v1.0\r\n");
	}
	else if (v == 0x92)
	{
		printf(" = v2.0\r\n");
	}
	else
	{
		printf(" (unknown)\r\n");
	}
	// When 0x00 or 0xFF is returned, communication probably failed
	if ((v == 0x00) || (v == 0xFF)) 
	{
		printf("WARNING: Communication failure, is the MFRC522 properly connected?\r\n");
		return false;
	}
	return true;
}

extern void CardReaderAppStart()
{
	SendBufferClear();
	hwCR_Init();
	PCD_Init();		// Init MFRC522
	isInit = ShowReaderDetails();	// Show details of PCD - MFRC522 Card Reader details
}


extern void CardReaderAppLoop()
{
	sSendInfo SendInfo;
	
	if (isInit == true)
	{
		// Look for new cards
		if ( ! PICC_IsNewCardPresent()) 
		{
			return;
		}
		printf("New card present\r\n");
		CountNewCard++;
		SendInfo.UtcDateTime = 0;
		SendInfo.Id = CountNewCard;
		memset(&SendInfo.CardUid, 0, sizeof(SendInfo.CardUid));
		// Select one of the cards
		if ( ! PICC_ReadCardSerial()) 
		{
			printf("Err read card serial\r\n");
			SendInfo.ReadResult = eCardReadErr;
		}
		else
		{
			SendInfo.ReadResult = eCardReadOk;
			SendInfo.CardUid = uid;
			PICC_DumpToSerial(&uid);	 // Dump debug info about the card; PICC_HaltA() is automatically called
		}
		SendBufferPut(SendInfo);
	}
	else
	{
		PCD_Init();		// Init MFRC522
		isInit = ShowReaderDetails();	// Show details of PCD - MFRC522 Card Reader details		
	}
}