#include "HwCardReader.h"


static i2c_config_t config;
static size_t TotalRead = 0;
static size_t IndxRead = 0;
static uint8_t DevAddr = 0;

static bool WaitFlag(i2c_flag_t flag, uint32_t timeout)
{
		uint32_t cycle = 0;
		flag_status_t status;
	
		do
		{
			status = i2c_get_flag_status(I2C0, flag);
			if (status != SET)
			{
				cycle++;
				delay_ms(1);
			}
		} while ((status != SET) && (cycle < timeout));
		if (status != SET)
		{
			return false;
		}
		return true;
}

void hwCR_Init(void)
{
    // set iomux
		gpio_init(GPIOA, GPIO_PIN_2, GPIO_MODE_OUTPUT_PP_HIGH);
    gpio_set_iomux(GPIOA, GPIO_PIN_14, 3);
    gpio_set_iomux(GPIOA, GPIO_PIN_15, 3);

    // init
    i2c_config_init(&config);
    i2c_init(I2C0, &config);
    i2c_cmd(I2C0, true);	
}

bool hwCR_Write(uint8_t addr, size_t count, uint8_t *data)
{
    // start
    i2c_master_send_start(I2C0, addr, I2C_WRITE);
	
    i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
		if (WaitFlag(I2C_FLAG_TRANS_EMPTY, 100) != true)
		{
			i2c_master_send_stop(I2C0);	
			return false;
		}

    // send data
    for(size_t i = 0; i < count; i++) 
		{
        i2c_send_data(I2C0, data[i]);
				if (WaitFlag(I2C_FLAG_TRANS_EMPTY, 100) != true)
				{
					i2c_master_send_stop(I2C0);	
					return false;
				}			
    }
    // stop
    i2c_master_send_stop(I2C0);	
		
		return true;
}

void hwCR_ReadBegin(uint8_t addr, size_t count)
{
		if (count != 1)
		{
			TotalRead = count;
			IndxRead = 0;
		}

		TotalRead = count;
		IndxRead = 0;
		DevAddr = addr;
}

bool hwCR_Read(uint8_t *data)
{
		*data = 0;
	
    i2c_master_send_start(I2C0, DevAddr, I2C_READ);
    i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
		if (WaitFlag(I2C_FLAG_TRANS_EMPTY, 100) != true)
		{
			i2c_master_send_stop(I2C0);	
			return false;
		}	
		i2c_set_receive_mode(I2C0, I2C_NAK);
		if (WaitFlag(I2C_FLAG_RECV_FULL, 100) != true)
		{
			i2c_master_send_stop(I2C0);	
			return false;
		}			
    *data = i2c_receive_data(I2C0);
		i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
		IndxRead++;
		return true;
}

bool hwCR_ReadAvailable()
{
		if (IndxRead < TotalRead)
		{
			return true;
		}
		return false;
}

void hwCR_ReadStop()
{
		i2c_master_send_stop(I2C0);
}

void hwCR_Reset()
{ 
		gpio_write(GPIOA, GPIO_PIN_2, GPIO_LEVEL_LOW); 
		delay_ms(50);
		gpio_write(GPIOA, GPIO_PIN_2, GPIO_LEVEL_HIGH);
		delay_ms(50);
}



