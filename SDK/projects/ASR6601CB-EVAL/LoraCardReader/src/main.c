#include <stdio.h>
#include "tremo_delay.h"
#include "debug.h"
#include "apps.h"
#include "rtc-board.h"
#include "tremo_pwr.h"

static void board_init()
{
    rcc_enable_oscillator(RCC_OSC_XO32K, true);

    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOA, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOB, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_GPIOD, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_UART0, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_PWR, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_RTC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_SAC, true);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LORA, true);
		rcc_enable_peripheral_clk(RCC_PERIPHERAL_I2C0, true);

    delay_ms(100);
    //pwr_xo32k_lpm_cmd(true);
		debug_init();
		printf("LoraWan card reader started\r\n");

    RtcInit();
}

int main(void)
{
	board_init();
	CardReaderAppStart();
	LoRaWanAppStart();
  while (1) 
	{
		//printf("Cycle\r\n");
		CardReaderAppLoop();
		LoRaWanAppLoop();
		delay_ms(100);
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
