#include <stdio.h>
#include "device.h"
#include "CardReader.h"

int main(void)
{
    /* Add your application code here */
		device_init();
    /* Infinite loop */
    while (1) 
		{
			printf("This is Card reader\r\n");
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
