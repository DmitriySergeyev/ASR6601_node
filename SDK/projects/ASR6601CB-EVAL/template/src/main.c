#include <stdio.h>
#include "CardReader.h"

void ShowReaderDetails() {
	// Get the MFRC522 software version
	uint8_t v = PCD_ReadRegister(VersionReg);
	printf("MFRC522 Software Version: 0x0%2X", v);
	if (v == 0x91)
		printf(" = v1.0\r\n");
	else if (v == 0x92)
		printf(" = v2.0\r\n");
	else
		printf(" (unknown)\r\n");
	// When 0x00 or 0xFF is returned, communication probably failed
	if ((v == 0x00) || (v == 0xFF)) {
		printf("WARNING: Communication failure, is the MFRC522 properly connected?\r\n");
	}
}


static void Start()
{
	PCD_Init();		// Init MFRC522
	ShowReaderDetails();	// Show details of PCD - MFRC522 Card Reader details
	printf("Scan PICC to see UID, type, and data blocks...\r\n");
}

static void Loop()
{
	printf("Cycle\r\n");
	// Look for new cards
	if ( ! PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards
	if ( ! PICC_ReadCardSerial()) {
		return;
	}

	// Dump debug info about the card; PICC_HaltA() is automatically called
	PICC_DumpToSerial(&uid);
}

int main(void)
{
    /* Add your application code here */
		device_init();
		printf("This is Card reader\r\n");
		//cr_pin_test();
		Start();
    /* Infinite loop */
    while (1) 
		{
			Loop();
			delay_ms(5000);
		}
}

#ifdef USE_FULL_ASSERT
void assert_failed(void* file, uint32_t line)
{
    (void)file;
    (void)line;

    while (1) { }
}
#endif
