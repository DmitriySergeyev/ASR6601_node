#ifndef APPS_H_
#define APPS_H_
#include "CardReader.h"
#include "syslog.h"

/* Функции и определения настроек */

#define SETTINGS_FLASH_ADDR    (0x0801F000)

#define LORA_EUI_LENGTH 8
#define LORA_KEY_LENGTH 16

typedef enum eJoinMode_t 
{
    JOIN_MODE_OTAA,
    JOIN_MODE_ABP
} eJoinMode;

typedef enum eAdrMode_t 
{
    ADR_MODE_ON,
    ADR_MODE_OFF,
} eAdrMode;

typedef struct sOTAKeys_t 
{
    uint8_t deveui[LORA_EUI_LENGTH];
    uint8_t appeui[LORA_EUI_LENGTH];
    uint8_t appkey[LORA_KEY_LENGTH];
} sOTAKeys;

typedef struct sABPKeys_t 
{
    uint32_t devaddr;
    uint8_t nwkskey[LORA_KEY_LENGTH];
    uint8_t appskey[LORA_KEY_LENGTH];
} sABPKeys;

typedef struct sPingDefs_t
{
	uint32_t Period;
	uint8_t NbTrials;
	uint8_t Port;
} sPingDefs;

typedef struct sSendDefs_t
{
	uint8_t NbTrials;
	uint8_t Port;
} sSendDefs;


typedef struct sDevSetting_t 
{
	eJoinMode JoinMode;
	eAdrMode AdrMode;
	sOTAKeys OTAKeys;
    sABPKeys ABPKeys;
	sPingDefs PingDefs;
	sSendDefs SendDefs;
} __attribute__((packed))sDevSetting;

typedef struct sDevSettingFile_t 
{
    uint32_t magic;
	sDevSetting DevSetting ;
    uint16_t checksum;
} __attribute__((packed))sDevSettingFile;

extern bool ReadSettings(sDevSetting *setting);
extern bool WriteSettings(sDevSetting *setting);

/* Функции хранения сообщений (буфер) */
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
extern sCardInfo SendBufferPop( );
extern void SendBufferDelete( );
extern uint16_t SendBufferGetCount( );
extern void SendBufferClear( );
extern uint8_t PrepareCardInfoFrame(sCardInfo Info, uint8_t *buff);

/* Функции приложения по чтению информации с карты */
extern void CardReaderAppStart();
extern void CardReaderAppLoop();
extern void CardReaderGetInfo(sCardReaderInfo *Info);

/*Функции приложения LoRa */
extern void LoRaWanAppStart();
extern void LoRaWanAppLoop();

#endif