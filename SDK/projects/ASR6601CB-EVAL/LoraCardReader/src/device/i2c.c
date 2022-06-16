#include "i2c.h"
#include "tremo_delay.h"

static i2c_config_t config;

void device_i2c_init(void)
{
    // enable the clk
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_I2C0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);

    // set iomux
    gpio_set_iomux(GPIOA, GPIO_PIN_14, 3);
    gpio_set_iomux(GPIOA, GPIO_PIN_15, 3);

    // init
    i2c_config_init(&config);
    i2c_init(I2C0, &config);
    i2c_cmd(I2C0, true);	
	
		delay_init();
}

size_t device_i2c_write(uint8_t addr, size_t count, uint8_t *data)
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

void device_i2c_begin_read(uint8_t addr)
{
    i2c_master_send_start(I2C0, addr, I2C_READ);
    i2c_clear_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY);
    while (i2c_get_flag_status(I2C0, I2C_FLAG_TRANS_EMPTY) != SET)
        ;	
		i2c_set_receive_mode(I2C0, I2C_NAK);
}

uint8_t device_i2c_read()
{
		uint8_t data;
	
    while (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) != SET)
        ;
    i2c_clear_flag_status(I2C0, I2C_FLAG_RECV_FULL);
    data = i2c_receive_data(I2C0);
		
		return data;
}

bool device_i2c_read_available()
{
		return (i2c_get_flag_status(I2C0, I2C_FLAG_RECV_FULL) == SET);
}

void device_i2c_stop_read()
{
		i2c_master_send_stop(I2C0);
}

void delay(uint32_t nms)
{
    delay_ms(nms);
}

void cr_reset()
{  
		gpio_init(GPIOA, GPIO_PIN_2, GPIO_MODE_OUTPUT_PP_HIGH);
		gpio_write(GPIOA, GPIO_PIN_2, GPIO_LEVEL_HIGH);
}


