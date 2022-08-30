#include "debug.h"

static uart_config_t uart_config;

void debug_init(void)
{
    // uart0
    gpio_set_iomux(GPIOB, GPIO_PIN_0, 1);
    gpio_set_iomux(GPIOB, GPIO_PIN_1, 1);

    /* uart config struct init */
    
    uart_config_init(&uart_config);

    uart_config.fifo_mode = ENABLE;
    uart_config.baudrate  = UART_BAUDRATE_115200;
    uart_init(CONFIG_DEBUG_UART, &uart_config);
    uart_cmd(CONFIG_DEBUG_UART, ENABLE);
    
    uart_config_interrupt(CONFIG_DEBUG_UART, UART_INTERRUPT_RX_DONE, ENABLE);
    uart_config_interrupt(CONFIG_DEBUG_UART, UART_INTERRUPT_RX_TIMEOUT, ENABLE);

    /* NVIC config */
    NVIC_EnableIRQ(UART0_IRQn);
}
