#include <stdio.h>
#include "device.h"

void device_init(void)
{
		// �������������� UART ��� ������ ����
    device_debug_init();
		printf("Debug uart init\r\n");
		// �������������� ���� ������
		printf("I2C init - ");
		device_i2c_init();
		printf("done\r\n");	
}