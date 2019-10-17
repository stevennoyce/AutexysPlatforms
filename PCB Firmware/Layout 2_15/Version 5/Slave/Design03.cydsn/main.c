#include "project.h"

#define SELECTOR_ID (3)

#define SELECTOR_COUNT (4u)
#define INTERMEDIATE_COUNT (4u)
#define CONTACT_COUNT (64u)
#define CONTACT_CONNECT_CODE (0xC0u)
#define CONTACT_DISCONNECT_CODE (0xD1u)

struct Selector_I2C_Struct {
	struct {
		uint8 subAddress;
		uint8 data[126];
	} write;
	struct {
		uint8 subAddress;
		uint8 data[126];
	} read;
	uint8 busAddress;
};

int main(void) {
	CyGlobalIntEnable;
	
	struct Selector_I2C_Struct selector;
	uint8 I2C_Bus_Addresses[SELECTOR_COUNT] = {0x66, 0x11, 0x44, 0x22};

	// Start all hardware blocks
	//AMux_1_Start(); //not required for a passive block
	Opamp_1_Start();
	ADC_SAR_Seq_1_Start();
	
	EZI2C_1_Start();
	EZI2C_1_EzI2CSetAddress1(I2C_Bus_Addresses[SELECTOR_ID]);
	EZI2C_1_EzI2CSetBuffer1(sizeof(selector), sizeof(selector), (uint8*) &selector);
	
	while(1) {
		for (uint8 i = 0; i < sizeof(selector.write.data); i++) {
			if (i < AMux_1_CHANNELS) {
				if (selector.write.data[i] == CONTACT_CONNECT_CODE) {
					AMux_1_Connect(i);
				}
				
				if (selector.write.data[i] == CONTACT_DISCONNECT_CODE) {
					AMux_1_Disconnect(i);
				}
			}
		}
	}
}


