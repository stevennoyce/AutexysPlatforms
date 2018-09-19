#include "project.h"

int main(void)
{
    uint8 i2cbuf[1];
	i2cbuf[0] = 0;
	
	//LED_Write(0);
	
	CyGlobalIntEnable;

    EZI2C_1_Start();
	EZI2C_1_EzI2CSetBuffer1(0x01, 0x01, i2cbuf);
	
    for(;;)
    {
        if (i2cbuf[0] == 0xbe) {
			//LED_Write(1);
		}
		if (!Button_Read()) {
			//LED_Write(0);
			//i2cbuf[0] = 0;
		}
    }
}
