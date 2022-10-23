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
	SYSLOG_D("MFRC522 Software Version: 0x%2X", ver);
	if (ver == 0x91)
	{
		SYSLOG_I("MFRC522 = v1.0");
	}
	else if (ver == 0x92)
	{
		SYSLOG_I("MFRC522 = v2.0");
	}
	else if (ver == 0xB2)
	{
		SYSLOG_I("Chinese clone");
	}	
	else
	{
		SYSLOG_I("MFRC522 version unknown");
	}
	// When 0x00 or 0xFF is returned, communication probably failed
	if ((ver == 0x00) || (ver == 0xFF)) 
	{
		SYSLOG_E("Communication failure, is the MFRC522 properly connected?");
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
		SYSLOG_I("New card present");
		CountNewCard++;
		CardInfo.UtcDateTime = (uint32_t)(RtcGetTimerValue() / 1000);
		CardInfo.Id = CountNewCard;
		memset(&CardInfo.CardUid, 0, sizeof(CardInfo.CardUid));
		// Select one of the cards
		if ( ! PICC_ReadCardSerial()) 
		{
			SYSLOG_E("Err read card serial");
			CardInfo.ReadResult = eCardReadErr;
		}
		else
		{
			CardInfo.ReadResult = eCardReadOk;
			CardInfo.CardUid = uid;
			PICC_DumpToSerial2(&uid);	 // Dump debug info about the card; PICC_HaltA() is automatically called
			hwCR_Beep();
		}
		SendBufferPut(CardInfo);
	}
	else
	{
		SYSLOG_I("Init MFRC522");
		PCD_Init();		// Init MFRC522
		isInit = ShowReaderDetails();	// Show details of PCD - MFRC522 Card Reader details		
	}
}

void CardReaderGetInfo(sCardReaderInfo *Info)
{
		Info->isInit = isInit;
		Info->ver = ver;
}
