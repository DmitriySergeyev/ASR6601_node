#include "HwCardReader.h"

static i2c_config_t config;
static size_t TotalRead = 0;
static size_t IndxRead = 0;
static uint8_t dev_addr = 0;

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

size_t hwCR_Write(uint8_t addr, size_t count, uint8_t *data)
{
    // start
    i2c_master_send_start(I2C0, addr, I2C_WRITE);
	
    i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
    while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET)
        ;

    // send data
    for(size_t i = 0; i < count; i++) 
		{
        i2c_send_data(I2C0, data[i]);

        i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
        while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET)
            ;
    }

    // stop
    i2c_master_send_stop(I2C0);	
		
	return count;
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
	dev_addr = addr;
}

uint8_t hwCR_Read()
{
	uint8_t data;
	
    i2c_master_send_start(I2C0, dev_addr, I2C_READ);
    i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
    while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET)
        ;	 	
	i2c_set_receive_mode(I2C0, I2C_NAK);
    while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET)
        ;
    
    data = i2c_receive_data(I2C0);
		i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
		IndxRead++;
		return data;
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



