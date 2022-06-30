#include "apps.h"
#include "HwCardReader.h"
#include "CardReader.h"
#include "rtc-board.h"

static bool isInit = false;
static uint32_t CountNewCard = 0;
static uint8_t ver = 0;

static bool ShowReaderDetails() 
{
	// Get the MFRC522 software version
	ver = PCD_ReadRegister(VersionReg);
	printf("MFRC522 Software Version: 0x0%2X", ver);
	if (ver == 0x91)
	{
		printf(" = v1.0\r\n");
	}
	else if (ver == 0x92)
	{
		printf(" = v2.0\r\n");
	}
	else
	{
		printf(" (unknown)\r\n");
	}
	// When 0x00 or 0xFF is returned, communication probably failed
	if ((ver == 0x00) || (ver == 0xFF)) 
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


void CardReaderAppLoop()
{
	sCardInfo CardInfo;
	
	if (isInit == true)
	{
		// Look for new cards
		if ( ! PICC_IsNewCardPresent()) 
		{
			return;
		}
		printf("New card present\r\n");
		CountNewCard++;
		CardInfo.UtcDateTime = (uint32_t)(RtcGetTimerValue() / 1000);
		CardInfo.Id = CountNewCard;
		memset(&CardInfo.CardUid, 0, sizeof(CardInfo.CardUid));
		// Select one of the cards
		if ( ! PICC_ReadCardSerial()) 
		{
			printf("Err read card serial\r\n");
			CardInfo.ReadResult = eCardReadErr;
		}
		else
		{
			CardInfo.ReadResult = eCardReadOk;
			CardInfo.CardUid = uid;
			PICC_DumpToSerial(&uid);	 // Dump debug info about the card; PICC_HaltA() is automatically called
		}
		SendBufferPut(CardInfo);
	}
	else
	{
		PCD_Init();		// Init MFRC522
		isInit = ShowReaderDetails();	// Show details of PCD - MFRC522 Card Reader details		
	}
}

void CardReaderGetInfo(sCardReaderInfo *Info)
{
		Info->isInit = isInit;
		Info->ver = ver;
}
