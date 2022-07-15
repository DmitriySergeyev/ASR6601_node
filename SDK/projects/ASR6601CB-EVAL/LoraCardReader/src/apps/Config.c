#include "apps.h"
#include "tremo_flash.h"
#include "Commissioning.h"

#define DEV_MAGIC_NUM 0xABABBABA 

static sDevSetting DefaultSetting = 
{
		.JoinMode = JOIN_MODE_OTAA,
		.OTAKeys.deveui = LORAWAN_DEVICE_EUI,
		.OTAKeys.appeui = LORAWAN_APPLICATION_EUI,
		.OTAKeys.appkey = LORAWAN_APPLICATION_KEY,
    .ABPKeys.devaddr = LORAWAN_DEVICE_ADDRESS,
    .ABPKeys.nwkskey = LORAWAN_NWKSKEY,
    .ABPKeys.appskey = LORAWAN_APPSKEY,	
		.PingDefs.Period = 60000,
		.PingDefs.NbTrials = 0,
		.PingDefs.Port = 2,
		.SendDefs.NbTrials = 0,
		.SendDefs.Port = 3,
};

static uint16_t crc16(uint8_t *buffer, uint8_t length )
{
    const uint16_t polynom = 0x1021;
    uint16_t crc = 0x0000;

    for (uint8_t i = 0; i < length; ++i) 
		{
        crc ^= ( uint16_t ) buffer[i] << 8;
        for (uint8_t j = 0; j < 8; ++j) 
				{
            crc = (crc & 0x8000) ? (crc << 1) ^ polynom : (crc << 1);
        }
    }

    return crc;
}

bool ReadSettings(sDevSetting *setting)
{
		sDevSettingFile File;
		uint16_t crc;
    
    memcpy(&File, (void *)SETTINGS_FLASH_ADDR, sizeof(sDevSettingFile));
		crc = crc16((uint8_t*)&File.DevSetting, sizeof(sDevSetting));
		SYSLOG_D("File.magic=0x%08lX, File.checksum=0x%08lX, Calc_checksum=0x%08lX", File.magic, File.checksum, crc);
		if ((File.magic != DEV_MAGIC_NUM) || (File.checksum != crc))
		{
				SYSLOG_W("Setting not load. Set default");
				*setting = DefaultSetting;
				return false;
		}
		SYSLOG_I("Setting load");
    *setting = File.DevSetting;
    return true;
}

bool WriteSettings(sDevSetting *setting)
{
		sDevSettingFile File;
    uint8_t buf[sizeof(sDevSettingFile)];
    int status = 0;

		File.magic = DEV_MAGIC_NUM;
		File.DevSetting = *setting;
		File.checksum = crc16((uint8_t*)&File.DevSetting, sizeof(sDevSetting));
	 
    memcpy(buf, &File, sizeof(File));
    flash_erase_page(SETTINGS_FLASH_ADDR);
    status = flash_program_bytes(SETTINGS_FLASH_ADDR, buf, sizeof(buf));
    if(status != 0)
		{
        SYSLOG_E("Error writing settings");
        return false;
    }
    return true;
}

