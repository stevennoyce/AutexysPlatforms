// === Imports ===
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "project.h"
#include "USBUART_Helpers.h"



// === Constants ===
// System Architecture
#define SELECTOR_COUNT (4u)
#define CONTACT_COUNT (64u)
#define CHANNEL_COUNT (34u)

// Device Selector Sub-system Communication
#define CONTACT_CONNECT_CODE (0xC0u)
#define CONTACT_DISCONNECT_CODE (0xD1u)
#define SELECTOR1_I2C_BUS_ADDRESS (0x66)
#define SELECTOR2_I2C_BUS_ADDRESS (0x11)
#define SELECTOR3_I2C_BUS_ADDRESS (0x44)
#define SELECTOR4_I2C_BUS_ADDRESS (0x22)
#define SELECTOR_SIGNAL_CHANNEL1 (33)
#define SELECTOR_SIGNAL_CHANNEL2 (34)
#define SELECTOR_SOURCE_TOP (1)
#define SELECTOR_SOURCE_BOTTOM (3)
#define SELECTOR_DRAIN_TOP (2)
#define SELECTOR_DRAIN_BOTTOM (4)

// Quantity and Locations of Feedback Resistors
#define TIA_INTERNAL_RESISTOR_COUNT (8)
#define AMUX_EXTERNAL_RESISTOR_COUNT (3)
#define AMUX_ADDRESS_FEEDBACK_R10K (0)
#define AMUX_ADDRESS_FEEDBACK_R1M (1)
#define AMUX_ADDRESS_FEEDBACK_R100M (2)

// Voltage Output DAC Parameters 
#define DAC_VOLTAGE_MAXIMUM (4.080)

// ADC De-noising Parameters
#define ADC_MEASUREMENT_MEDIAN_CHUNKSIZE (9)
#define ADC_MEASUREMENT_MEAN_CHUNKSIZE (15)
#define SAR_MEASUREMENT_MEDIAN_CHUNKSIZE (9)
#define SAR_MEASUREMENT_MEAN_CHUNKSIZE (30)
#define ADC_CALIBRATION_SAMPLECOUNT (100)
#define AUTO_RANGE_SAMPLECOUNT (3)
#define AUTO_RANGE_DISCARDCOUNT (0)
#define DRAIN_CURRENT_MEASUREMENT_DISCARDCOUNT (0)
#define GATE_CURRENT_MEASUREMENT_DISCARDCOUNT (0)

// Primary Measurement Parameters
#define DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT (45)
#define GATE_CURRENT_MEASUREMENT_SAMPLECOUNT (90)
#define ADC_INCREASE_RANGE_THRESHOLD (1024000 * 1.00) //ADC does not saturate until +/- 1.126 V, but "safe" range is up to +/- 1.024 V -- see datasheet for more info
#define ADC_DECREASE_RANGE_THRESHOLD (1024000 * 0.0085)

// Compliance Current
#define COMPLIANCE_CURRENT_LIMIT (10e-6)



// === Struct definitions ===
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



// === Communication Variables === 
// Serial USB/Bluetooth output buffer
char TransmitBuffer[USBUART_BUFFER_SIZE];

// Bluetooth input buffer
volatile char UART_Receive_Buffer[USBUART_BUFFER_SIZE];
volatile uint8 UART_Rx_Position;

// USB input buffer
volatile char USBUART_Receive_Buffer[USBUART_BUFFER_SIZE];
volatile uint8 USBUART_Rx_Position;

// Enable/disable communication
bool uartSendingEnabled = false; //Bluetooth
bool usbuSendingEnabled = true; //USB

// Global flags for stopping or pausing long procedures
volatile uint8 G_Stop = 0;
volatile uint8 G_Break = 0;
volatile uint8 G_Pause = 0;

// Communication flag (non-zero when receiving a new command [1 == USB, 2 == Bluetooth])
volatile uint8 newData = 0;



// === System Variables === 
// Device Selectors
struct Selector_I2C_Struct selectors[SELECTOR_COUNT];

// Internal state to track currently-selected TIA Feedback Resistor
enum TIA_resistor {Internal_R20K, Internal_R30K, Internal_R40K, Internal_R80K, Internal_R120K, Internal_R250K, Internal_R500K, Internal_R1000K, External_R10K, External_R1M, External_R100M};
uint8 TIA_Selected_Resistor = External_R1M;

// TIA Resistor Properties (access codes, impedance values, input offset voltages)
uint8 TIA_Resistor_Codes[TIA_INTERNAL_RESISTOR_COUNT + AMUX_EXTERNAL_RESISTOR_COUNT] = {TIA_1_RES_FEEDBACK_20K, TIA_1_RES_FEEDBACK_30K, TIA_1_RES_FEEDBACK_40K, TIA_1_RES_FEEDBACK_80K, TIA_1_RES_FEEDBACK_120K, TIA_1_RES_FEEDBACK_250K, TIA_1_RES_FEEDBACK_500K, TIA_1_RES_FEEDBACK_1000K, AMUX_ADDRESS_FEEDBACK_R10K, AMUX_ADDRESS_FEEDBACK_R1M, AMUX_ADDRESS_FEEDBACK_R100M};
uint8 TIA_Resistor_Enabled[TIA_INTERNAL_RESISTOR_COUNT + AMUX_EXTERNAL_RESISTOR_COUNT] = {true, false, false, false, false, false, false, true, true, true, true};
float TIA_Resistor_Values[TIA_INTERNAL_RESISTOR_COUNT + AMUX_EXTERNAL_RESISTOR_COUNT] = {20e3, 30e3, 40e3, 80e3, 120e3, 250e3, 500e3, 1e6, 10e3, 1e6, 100e6};
int32 TIA_Offsets_uV[TIA_INTERNAL_RESISTOR_COUNT + AMUX_EXTERNAL_RESISTOR_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Setpoints for VGS and VDS (in the signed 9-bit format that the DACs use)
int16 Vgs_Index_Goal_Relative;
int16 Vds_Index_Goal_Relative;

// Alert that maximum current limit has been exceeded
uint8 Compliance_Reached;



// === Section: Device Selectors & Communication ===

// Print output message to USB and Bluetooth (if both are enabled)
void sendTransmitBuffer() {
	if (usbuSendingEnabled) USBUARTH_Send(TransmitBuffer, strlen(TransmitBuffer));
	if (uartSendingEnabled) UART_1_PutString(TransmitBuffer);
}

// SETUP: initialize the internal data structures that are used for communication with a device selector using I2C communication
void Setup_Selector_I2C_Struct(struct Selector_I2C_Struct *selector) {
	selector->write.subAddress = 1;
	selector->read.subAddress = sizeof(selector->write) + 1;
	
	// Initialize the Device Selector's "write" buffer (these bits control the AMUX)
	for (uint16 i = 0; i < sizeof(selector->write.data); i++) {
		selector->write.data[i] = i + 6;
	}
	
	// Initialize the Device Selector's "read" buffer (these are bits that the Device Selector can use to send messages to the Master CPU)
	for (uint16 i = 0; i < sizeof(selector->read.data); i++) {
		selector->read.data[i] = 0xbe;
	}
}

// SETUP: set up all communication data structures for device selectors
void Setup_Selectors() {
	for (uint8 i = 0; i < SELECTOR_COUNT; i++) Setup_Selector_I2C_Struct(&selectors[i]);
	
	selectors[0].busAddress = SELECTOR1_I2C_BUS_ADDRESS;
	selectors[1].busAddress = SELECTOR2_I2C_BUS_ADDRESS;
	selectors[2].busAddress = SELECTOR3_I2C_BUS_ADDRESS;
	selectors[3].busAddress = SELECTOR4_I2C_BUS_ADDRESS;
}

// Use I2C communication to tell a selector to update its connections
void Update_Selector(uint8 selector_index) {
	// Get reference to selector communication data structure for this selector index
	struct Selector_I2C_Struct* selector = &selectors[selector_index];
	
	sprintf(TransmitBuffer, "Updating Selector %u (addr: 0x%02x)...\r\n", selector_index + 1, selector->busAddress);
	sendTransmitBuffer();
	
	// Clear any previous alerts and send new communication onto the I2C bus
	I2C_1_MasterClearStatus();
	I2C_1_MasterWriteBuf(selector->busAddress, (uint8 *) &selector->write, sizeof(selector->write), I2C_1_MODE_COMPLETE_XFER);
	
	// Large for-loop that serves as a time-out in case communication fails
	for (uint32 i = 0; i < 4e5; i++) {
		if ((I2C_1_MasterStatus() & I2C_1_MSTAT_WR_CMPLT)) break;
		if (i >= 4e5 - 1) {
			sprintf(TransmitBuffer, "I2C Transfer Error! Type: Timeout\r\n");
			sendTransmitBuffer();
			I2C_1_Stop();
			I2C_1_Start();
		}
	}
	
	// Tell Device Selector to send the data from its read buffer
	// I2C_1_MasterClearStatus();
	// I2C_1_MasterWriteBuf(selector->busAddress, (uint8 *) &selector->read.subAddress, 1, I2C_1_MODE_NO_STOP);
	// for (uint32 i = 0; i < 4e5; i++) {
	// 	if ((I2C_1_MasterStatus() & I2C_1_MSTAT_WR_CMPLT)) break;
	// }
	
	// Read data from the Device Selector
	// I2C_1_MasterReadBuf(selector->busAddress, (uint8 *) &selector->read.data, sizeof(selector->read.data), I2C_1_MODE_REPEAT_START);
	// for (uint32 i = 0; i < 4e5; i++) {
	// 	if ((I2C_1_MasterStatus() & I2C_1_MSTAT_RD_CMPLT)) break;
	// }
	
	// Check for any errors that could have occurred
	if (I2C_1_MasterStatus() & I2C_1_MSTAT_ERR_XFER) {
		if(I2C_1_MSTAT_ERR_ADDR_NAK) {
			sprintf(TransmitBuffer, "I2C Transfer Error! Type: NAK\r\n");
			sendTransmitBuffer();
		} else {
			sprintf(TransmitBuffer, "I2C Transfer Error! \r\n");
			sendTransmitBuffer();
		}
	}
	
	sprintf(TransmitBuffer, "Updated Selector %u.\r\n", selector_index + 1);
	sendTransmitBuffer();
}

// CONNECT: Make a connection on one of the 34 channels in a device selector's analog mux
void Connect_Channel_On_Selector(uint8 channel, uint8 selector_index) {
	sprintf(TransmitBuffer, "Connecting channel %u::%u.\r\n", selector_index, channel);
	sendTransmitBuffer();
	
	channel--;
	selector_index--;
	
	if ((channel >= CHANNEL_COUNT) || (selector_index >= SELECTOR_COUNT)) return;
	
	selectors[selector_index].write.data[channel] = CONTACT_CONNECT_CODE;
	Update_Selector(selector_index);
}

// DISCONNECT: Break a connection on one of the 34 channels in a device selector's analog mux
void Disconnect_Channel_On_Selector(uint8 channel, uint8 selector_index) {
	sprintf(TransmitBuffer, "Disconnecting channel %u::%u.\r\n", selector_index, channel);
	sendTransmitBuffer();
	
	channel--;
	selector_index--;
	
	if ((channel >= CHANNEL_COUNT) || (selector_index >= SELECTOR_COUNT)) return;
	
	selectors[selector_index].write.data[channel] = CONTACT_DISCONNECT_CODE;
	Update_Selector(selector_index);
}

// CONNECT: Make a connection on one of the 32 device contact signals available to a device selector
void Connect_Contact_To_Selector(uint8 contact, uint8 selector_index) {
	contact--;
	selector_index--;
	
	if ((contact >= CONTACT_COUNT) || (selector_index >= SELECTOR_COUNT)) return;
	
	uint8 offset = 0;
	if (selector_index >= SELECTOR_COUNT/2) {
		offset = CONTACT_COUNT/2;
	}
	
	if ((contact < offset) || (contact - offset >= CONTACT_COUNT/2)) return;
	
	Connect_Channel_On_Selector(contact - offset + 1, selector_index + 1);
}

// DISCONNECT: Break a connection on one of the 32 device contact signals available to a device selector
void Disconnect_Contact_From_Selector(uint8 contact, uint8 selector_index) {
	contact--;
	selector_index--;
	
	if ((contact >= CONTACT_COUNT) || (selector_index >= SELECTOR_COUNT)) return;
	
	uint8 offset = 0;
	if (selector_index >= SELECTOR_COUNT/2) {
		offset = CONTACT_COUNT/2;
	}
	
	Disconnect_Channel_On_Selector(contact - offset + 1, selector_index + 1);
}

// DISCONNECT: Break a specific contact connection on all device selectors
void Disconnect_Contact_From_All_Selectors(uint8 contact) {
	for (uint8 selector_index = 1; selector_index <= SELECTOR_COUNT; selector_index++) {
		Disconnect_Contact_From_Selector(contact, selector_index);
	}
}

// DISCONNECT: Break all contact connections on a specific device selector (note: does not affect channel 33 or 34)
void Disconnect_All_Contacts_From_Selector(uint8 selector_index) {
	selector_index--;
	
	for (uint8 contact = 0; contact < CONTACT_COUNT/2; contact++) {
		selectors[selector_index].write.data[contact] = CONTACT_DISCONNECT_CODE;
	}
	
	Update_Selector(selector_index);
}

// DISCONNECT: Break all contact connections on all device selectors (note: does not affect channel 33 or 34)
void Disconnect_All_Contacts_From_All_Selectors() {
	for (uint8 selector_index = 1; selector_index <= SELECTOR_COUNT; selector_index++) {
		Disconnect_All_Contacts_From_Selector(selector_index);
	}
}

// CONNECT: Connect this device contact to the source-side of Vds
uint8 Connect_Contact_To_Source(uint8 contact) {
	uint8 selector_index = (contact <= CONTACT_COUNT/2) ? (SELECTOR_SOURCE_TOP) : (SELECTOR_SOURCE_BOTTOM);
	Connect_Contact_To_Selector(contact, selector_index);
	return selector_index;
}

// CONNECT: Connect this device contact to the drain-side of Vds
uint8 Connect_Contact_To_Drain(uint8 contact) {
	uint8 selector_index = (contact <= CONTACT_COUNT/2) ? (SELECTOR_DRAIN_TOP) : (SELECTOR_DRAIN_BOTTOM);
	Connect_Contact_To_Selector(contact, selector_index);
	return selector_index;
}

// CONNECT: Makes the connection on the primary source/drain-signal channel (DD1 for selectors 1 & 3, SS1 for selectors 2 & 4)
void Connect_Selector_Signal_Channel1(uint8 selector_index) {
	sprintf(TransmitBuffer, "Connecting selector %u to primary signal channel.\r\n", selector_index);
	sendTransmitBuffer();
	switch (selector_index) {
		case 1: Connect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL1, 1); break;
		case 2: Connect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL1, 2); break;
		case 3: Connect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL1, 3); break;
		case 4: Connect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL1, 4); break;
		default: return;
	}
}

// DISCONNECT: Breaks the connection on the primary drain/source-signal channel (DD1 for selectors 1 & 3, SS1 for selectors 2 & 4)
void Disconnect_Selector_Signal_Channel1(uint8 selector_index) {
	sprintf(TransmitBuffer, "Disconnecting selector %u from primary signal channel.\r\n", selector_index);
	sendTransmitBuffer();
	switch (selector_index) {
		case 1: Disconnect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL1, 1); break;
		case 2: Disconnect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL1, 2); break;
		case 3: Disconnect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL1, 3); break;
		case 4: Disconnect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL1, 4); break;
		default: return;
	}
}

// CONNECT: Makes the connection on the backup source/drain-signal channel (DD2 for selectors 1 & 3, SS2 for selectors 2 & 4)
void Connect_Selector_Signal_Channel2(uint8 selector_index) {
	sprintf(TransmitBuffer, "Connecting selector %u to secondary signal channel.\r\n", selector_index);
	sendTransmitBuffer();
	switch (selector_index) {
		case 1: Connect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL2, 1); break;
		case 2: Connect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL2, 2); break;
		case 3: Connect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL2, 3); break;
		case 4: Connect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL2, 4); break;
		default: return;
	}
}

// DISCONNECT: Breaks the connection on the backup drain/source-signal channel (DD2 for selectors 1 & 3, SS2 for selectors 2 & 4)
void Disconnect_Selector_Signal_Channel2(uint8 selector_index) {
	sprintf(TransmitBuffer, "Disconnecting selector %u from secondary signal channel.\r\n", selector_index);
	sendTransmitBuffer();
	switch (selector_index) {
		case 1: Disconnect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL2, 1); break;
		case 2: Disconnect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL2, 2); break;
		case 3: Disconnect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL2, 3); break;
		case 4: Disconnect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL2, 4); break;
		default: return;
	}
}

// CONNECT: Connects the primary source/drain-signal channels to all device selectors
void Connect_Selectors_To_Primary_Signal_Channel() {
	for (uint8 i = 1; i <= SELECTOR_COUNT; i++) {
		Connect_Selector_Signal_Channel1(i);
	}
}

// CONNECT: Connects the backup source/drain-signal channels to all device selectors
void Connect_Selectors_To_Secondary_Signal_Channel() {
	for (uint8 i = 1; i <= SELECTOR_COUNT; i++) {
		Connect_Selector_Signal_Channel2(i);
	}
}

// DISCONNECT: Disconnects the source/drain-signal channels from all device selectors
void Disconnect_Selectors() {
	for (uint8 i = 1; i <= SELECTOR_COUNT; i++) {
		Disconnect_Selector_Signal_Channel1(i);
		Disconnect_Selector_Signal_Channel2(i);
	}
}

// CONNECT: Connects the backup source/drain-signal channels to all device selectors
void Switch_Selectors_To_Signal_Channel(uint8 signal_channel) {
	Disconnect_Selectors();
	if(signal_channel == 1) {
		Connect_Selectors_To_Primary_Signal_Channel();
	} else if(signal_channel == 2) {
		Connect_Selectors_To_Secondary_Signal_Channel();
	}
}

// === End Section: Device Selectors & Communication ===



// === Section: ADC Range Selection ===

// Change the feedback resistor used in current measurement circuit
void TIA_Set_Resistor(uint8 resistor) {
	TIA_Selected_Resistor = resistor;
	if(resistor < TIA_INTERNAL_RESISTOR_COUNT) {
		TIA_1_SetResFB(TIA_Resistor_Codes[resistor]);
	} else {
		AMux_1_Select(TIA_Resistor_Codes[resistor]);
	}
}

// Shift feedback resistance to allow ADC to measure larger currents
void ADC_Increase_Range() {
	switch(TIA_Selected_Resistor) {
		case Internal_R20K: 	 break;
		//case Internal_R30K: 	 TIA_Set_Resistor(Internal_R20K); break;
		//case Internal_R40K:	 TIA_Set_Resistor(Internal_R30K); break;
		//case Internal_R80K:	 TIA_Set_Resistor(Internal_R40K); break;
		//case Internal_R120K:	 TIA_Set_Resistor(Internal_R80K); break;
		//case Internal_R250K:	 TIA_Set_Resistor(Internal_R120K); break;
		//case Internal_R500K:	 TIA_Set_Resistor(Internal_R250K); break;
		case Internal_R1000K:    TIA_Set_Resistor(Internal_R20K); break;
		case External_R10K:      break;
		case External_R1M:       TIA_Set_Resistor(External_R10K); break;
		case External_R100M:     TIA_Set_Resistor(External_R1M); break;
		default: return;
	}
}

// Shift feedback resistance to allow ADC to measure smaller currents
void ADC_Decrease_Range() {
	switch(TIA_Selected_Resistor) {
		case Internal_R20K: 	 TIA_Set_Resistor(Internal_R1000K); break;
		//case Internal_R30K: 	 TIA_Set_Resistor(Internal_R40K); break;
		//case Internal_R40K:	 TIA_Set_Resistor(Internal_R80K); break;
		//case Internal_R80K:	 TIA_Set_Resistor(Internal_R120K); break;
		//case Internal_R120K:	 TIA_Set_Resistor(Internal_R250K); break;
		//case Internal_R250K:	 TIA_Set_Resistor(Internal_R500K); break;
		//case Internal_R500K:	 TIA_Set_Resistor(Internal_R1000K); break;
		case Internal_R1000K:    break;
		case External_R10K:      TIA_Set_Resistor(External_R1M); break;
		case External_R1M:       TIA_Set_Resistor(External_R100M); break;
		case External_R100M:     break;
		default: return;
	}
}

// === End Section: ADC Range Selection ===



// === Section: ADC Measurement ===

// HELPER: sort array so that we can take the median
void sortArray(int32 array[], uint32 size) {
	int32 temp;
	// the following two loops sort the array x in ascending order
	for(uint32 i = 0; i < size-1; i++) {
		for(uint32 j = i+1; j < size; j++) {
			if(array[j] < array[i]) {
				// swap elements
				temp = array[i];
				array[i] = array[j];
				array[j] = temp;
			}
		}
	}
}

// HELPER: sort and get the median value of an array
int32 medianOfArray(int32 array[], uint32 size) {
	sortArray(array, size);
	
	if(size%2 == 0) {
		// if size is even, return the mean of middle elements
		return((array[size/2] + array[size/2 - 1]) / 2);
	} else {
		// else return the middle element
		return array[size/2];
	}
}

// HELPER: get the mean of an array
float meanOfArray(int32 array[], uint32 size) {
	int32 sum = 0;
	
	for (uint32 i = 0; i < size; i++) {
		sum += array[i];
	}
	
	return ((float)sum)/((float)size);
}

// MEASURE: Measure Delta-Sigma ADC
void ADC_Means_Measure_uV(int32* average, uint32 sampleCount) {
	if(sampleCount == 0) return;
	
	// Samples will be sorted, allowing extreme values to be discarded, and the mean of the remaining middle chunk gives the final result
	uint32 meanArraySize = ADC_MEASUREMENT_MEAN_CHUNKSIZE;
	if(sampleCount < meanArraySize) meanArraySize = sampleCount;
	uint32 discardOffset = (sampleCount - meanArraySize)/2;
	
	// Define arrays
	int32 sampleArray[sampleCount];
	int32 meanArray[meanArraySize];
	
	for (uint32 i = 0; i < sampleCount; i++) {
		while (!ADC_DelSig_1_IsEndConversion(ADC_DelSig_1_RETURN_STATUS));
		sampleArray[i] = ADC_DelSig_1_CountsTo_uVolts(ADC_DelSig_1_GetResult32());
	}
	
	// Sort and discard samples at high and low extremes, keeping the middle chunk of values
	sortArray(sampleArray, sampleCount);
	for (uint32 i = 0; i < meanArraySize; i++) {
		meanArray[i] = sampleArray[i+discardOffset];
	}
	
	// Average middle chunk of values for the final result
	int32 ADC_Result = meanOfArray(meanArray, meanArraySize);
	
	*average = ADC_Result;
}

// MEASURE: Measure Delta-Sigma ADC
void ADC_Medians_Measure_uV(int32* average, int32* standardDeviation, uint32 sampleCount) {
	if(sampleCount == 0) return;
	
	int32 ADC_Result = 0;
	int32 ADC_SD = 0;
	
	// Split measurements into chunks, taking the median of each chunk and averaging those medians for the final result
	uint32 medianArraySize = ADC_MEASUREMENT_MEDIAN_CHUNKSIZE;
	if (sampleCount < medianArraySize) medianArraySize = sampleCount;
	
	uint32 medianCount = sampleCount/medianArraySize;
	int32 medianArray[medianArraySize];
	
	for (uint32 i = 1; i <= medianCount; i++) {
		for (uint32 j = 0; j < medianArraySize; j++) {
			while (!ADC_DelSig_1_IsEndConversion(ADC_DelSig_1_RETURN_STATUS));
			//int16 ADC_Result = ADC_DelSig_1_CountsTo_mVolts(ADC_DelSig_1_GetResult16());
			medianArray[j] = ADC_DelSig_1_CountsTo_uVolts(ADC_DelSig_1_GetResult32());
		}
		
		int32 ADC_Result_Current = medianOfArray(medianArray, medianArraySize);
		
		ADC_SD += (float)(i-1)/(float)(i)*(ADC_Result_Current - ADC_Result)*(ADC_Result_Current - ADC_Result);
		ADC_Result += ((float)ADC_Result_Current - (float)ADC_Result)/(float)i;
	}
	
	*average = ADC_Result;
	*standardDeviation = ADC_SD;
}

// MEASURE: Measure Delta-Sigma ADC
void ADC_Measure_uV(int32* average, int32* standardDeviation, uint32 sampleCount) {
	ADC_Means_Measure_uV(average, sampleCount);
	//ADC_Medians_Measure_uV(average, standardDeviation, sampleCount);
}

// RANGE: take a test measurement of the delta-sigma ADC and adjust its range if necessary
void ADC_Adjust_Range(uint32 sampleCount) {
	if(sampleCount == 0) return;
	
	int32 ADC_Voltage = 0;
	int32 ADC_Voltage_SD = 0;
	
	// Take ADC measurement to see what range we should be in
	ADC_Measure_uV(&ADC_Voltage, &ADC_Voltage_SD, sampleCount);
	
	// Determine if we need to change the range
	if(abs(ADC_Voltage) > ADC_INCREASE_RANGE_THRESHOLD) {
		ADC_Increase_Range();
	} else if (abs(ADC_Voltage) < ADC_DECREASE_RANGE_THRESHOLD) {
		ADC_Decrease_Range();
	} else {
		return;
	}
	
	// If the range switches, discard a few measurements
	ADC_Measure_uV(&ADC_Voltage, &ADC_Voltage_SD, AUTO_RANGE_DISCARDCOUNT);
}

// MEASURE: Measure SAR1 ADC (using primarily means)
void SAR1_Means_Measure_uV(int32* average, uint32 sampleCount) {
	if(sampleCount == 0) return;
	
	// Samples will be sorted, allowing extreme values to be discarded, and the mean of the remaining middle chunk gives the final result
	uint32 meanArraySize = SAR_MEASUREMENT_MEAN_CHUNKSIZE;
	if(sampleCount < meanArraySize) meanArraySize = sampleCount;
	uint32 discardOffset = (sampleCount - meanArraySize)/2;
	
	// Define arrays
	int32 sampleArray[sampleCount];
	int32 meanArray[meanArraySize];
	
	// ** Only defined if SAR_1 is enabled in the design
	#ifdef ADC_SAR_1_RETURN_STATUS
	//ADC_SAR_1_StartConvert();
	for (uint32 i = 0; i < sampleCount; i++) {
		while (!ADC_SAR_1_IsEndConversion(ADC_SAR_1_RETURN_STATUS));
		sampleArray[i] = ADC_SAR_1_CountsTo_uVolts(ADC_SAR_1_GetResult16());
	}
	//ADC_SAR_1_StopConvert();
	#endif
	
	// Sort and discard samples at high and low extremes, keeping the middle chunk of values
	sortArray(sampleArray, sampleCount);
	for (uint32 i = 0; i < meanArraySize; i++) {
		meanArray[i] = sampleArray[i+discardOffset];
	}
	
	// Average middle chunk of values for the final result
	int32 SAR_Result = meanOfArray(meanArray, meanArraySize);
	
	*average = SAR_Result;
}

// MEASURE: Measure SAR1 ADC (using primarily medians)
void SAR1_Medians_Measure_uV(int32* average, int32* standardDeviation, uint32 sampleCount) {
	if(sampleCount == 0) return;
	
	int32 SAR_Result = 0;
	int32 SAR_SD = 0;
	
	// Split measurements into chunks, taking the median of each chunk and averaging those medians for the final result
	uint32 medianArraySize = SAR_MEASUREMENT_MEDIAN_CHUNKSIZE;
	if (sampleCount < medianArraySize) medianArraySize = sampleCount;
	
	uint32 medianCount = sampleCount/medianArraySize;
	int32 medianArray[medianArraySize];
	
	// ** Only defined if SAR_1 is enabled in the design
	#ifdef ADC_SAR_1_RETURN_STATUS
	//ADC_SAR_1_StartConvert();
	for (uint32 i = 1; i <= medianCount; i++) {
		for (uint32 j = 0; j < medianArraySize; j++) {
			while (!ADC_SAR_1_IsEndConversion(ADC_SAR_1_RETURN_STATUS));
			medianArray[j] = ADC_SAR_1_CountsTo_uVolts(ADC_SAR_1_GetResult16());
		}
		
		int32 SAR1_Result_Current = medianOfArray(medianArray, medianArraySize);
		
		SAR_SD += (float)(i-1)/(float)(i)*(SAR1_Result_Current - SAR_Result)*(SAR1_Result_Current - SAR_Result);
		SAR_Result += ((float)SAR1_Result_Current - (float)SAR_Result)/(float)i;
	}
	//ADC_SAR_1_StopConvert();
	#endif
	
	*average = SAR_Result;
	*standardDeviation = SAR_SD;
}

// MEASURE: Measure SAR1 ADC
void SAR1_Measure_uV(int32* average, int32* standardDeviation, uint32 sampleCount) {
	SAR1_Means_Measure_uV(average, sampleCount);
	//SAR1_Medians_Measure_uV(average, standardDeviation, sampleCount);
}

// MEASURE: Measure SAR1 ADC (using primarily means)
void SAR2_Means_Measure_uV(int32* average, uint32 sampleCount) {
	if(sampleCount == 0) return;
	
	// Samples will be sorted, allowing extreme values to be discarded, and the mean of the remaining middle chunk gives the final result
	uint32 meanArraySize = SAR_MEASUREMENT_MEAN_CHUNKSIZE;
	if(sampleCount < meanArraySize) meanArraySize = sampleCount;
	uint32 discardOffset = (sampleCount - meanArraySize)/2;
	
	// Define arrays
	int32 sampleArray[sampleCount];
	int32 meanArray[meanArraySize];
	
	// ** Only defined if SAR_2 is enabled in the design
	#ifdef ADC_SAR_2_RETURN_STATUS
	//ADC_SAR_2_StartConvert();
	for (uint32 i = 0; i < sampleCount; i++) {
		while (!ADC_SAR_2_IsEndConversion(ADC_SAR_2_RETURN_STATUS));
		sampleArray[i] = ADC_SAR_2_CountsTo_uVolts(ADC_SAR_2_GetResult16());
	}
	//ADC_SAR_2_StopConvert();
	#endif
	
	// Sort and discard samples at high and low extremes, keeping the middle chunk of values
	sortArray(sampleArray, sampleCount);
	for (uint32 i = 0; i < meanArraySize; i++) {
		meanArray[i] = sampleArray[i+discardOffset];
	}
	
	// Average middle chunk of values for the final result
	int32 SAR_Result = meanOfArray(meanArray, meanArraySize);
	
	*average = SAR_Result;
}

// MEASURE: Measure SAR2 ADC (using primarily medians)
void SAR2_Medians_Measure_uV(int32* average, int32* standardDeviation, uint32 sampleCount) {
	if(sampleCount == 0) return;
	
	int32 SAR_Result = 0;
	int32 SAR_SD = 0;
	
	// Split measurements into chunks, taking the median of each chunk and averaging those medians for the final result
	uint32 medianArraySize = SAR_MEASUREMENT_MEDIAN_CHUNKSIZE;
	if (sampleCount < medianArraySize) medianArraySize = sampleCount;
	
	uint32 medianCount = sampleCount/medianArraySize;
	int32 medianArray[medianArraySize];
	
	// ** Only defined if SAR_2 is enabled in the design
	#ifdef ADC_SAR_2_RETURN_STATUS 
	//ADC_SAR_2_StartConvert();
	for (uint32 i = 1; i <= medianCount; i++) {
		for (uint32 j = 0; j < medianArraySize; j++) {
			while (!ADC_SAR_2_IsEndConversion(ADC_SAR_2_RETURN_STATUS));
			medianArray[j] = ADC_SAR_2_CountsTo_uVolts(ADC_SAR_2_GetResult16());
		}
		
		int32 SAR2_Result_Current = medianOfArray(medianArray, medianArraySize);
		
		SAR_SD += (float)(i-1)/(float)(i)*(SAR2_Result_Current - SAR_Result)*(SAR2_Result_Current - SAR_Result);
		SAR_Result += ((float)SAR2_Result_Current - (float)SAR_Result)/(float)i;
	}
	//ADC_SAR_2_StopConvert();
	#endif
	
	*average = SAR_Result;
	*standardDeviation = SAR_SD;
}

// MEASURE: Measure SAR2 ADC
void SAR2_Measure_uV(int32* average, int32* standardDeviation, uint32 sampleCount) {
	SAR2_Means_Measure_uV(average, sampleCount);
	//SAR2_Medians_Measure_uV(average, standardDeviation, sampleCount);
}

// MEASURE: Measure drain current
void Measure_Drain_Current(float* currentAverageIn, float* currentStdDevIn, uint32 sampleCount) {		
	// Auto-adjust the current range
	ADC_Adjust_Range(AUTO_RANGE_SAMPLECOUNT);
	
	// After auto-ranging completes, determine voltage-current conversion from the chosen feedback resistance 
	float TIA1_Feedback_R = TIA_Resistor_Values[TIA_Selected_Resistor];
	int32 TIA1_Offset_uV = TIA_Offsets_uV[TIA_Selected_Resistor];
	float unitConversion = -1.0e-6/TIA1_Feedback_R;
	//unitConversion = -1.0e-6/100e6; //override internal TIA resistor value here
	
	// Voltage and its standard deviation (in uV)
	int32 ADC_Voltage = 0;
	int32 ADC_Voltage_SD = 0;
	
	// Current and its standard deviation (in A)
	float currentAverage = 0;
	float currentStdDev = 0;
	
	// Allow for the first measurement (normally not correct) to take place
	int32 discard = 0;
	ADC_Measure_uV(&discard, &discard, DRAIN_CURRENT_MEASUREMENT_DISCARDCOUNT);
	
	// Now take the real ADC measurement
	ADC_Measure_uV(&ADC_Voltage, &ADC_Voltage_SD, sampleCount);
	
	// Convert from ADC microvolts to a measurement of current in amperes (using the value of feedback resistance plus a calibration factor)
	currentAverage = unitConversion * (ADC_Voltage + TIA1_Offset_uV);
	currentStdDev = unitConversion * (ADC_Voltage_SD + TIA1_Offset_uV);
	
	*currentAverageIn = currentAverage;
	*currentStdDevIn = currentStdDev;
}

// MEASURE: Measure gate current
void Measure_Gate_Current(float* currentAverageIn, float* currentStdDevIn, uint32 sampleCount) {
	// Determine voltage-current conversion from resistance being used in the measurement
	float Vgs_DAC_Series_R = 1e6; //Vgs DAC has a permanent 1 megaohm current-limiting resistor
	float unitConversion = 1.0e-6/Vgs_DAC_Series_R;
	
	// Voltage and its standard deviation (in uV)
	int32 SAR_Voltage = 0;
	int32 SAR_Voltage_SD = 0;
	
	// Current and its standard deviation (in A)
	float currentAverage = 0;
	float currentStdDev = 0;
	
	// Allow for the first measurement (normally not correct) to take place
	int32 discard = 0;
	SAR1_Measure_uV(&discard, &discard, GATE_CURRENT_MEASUREMENT_DISCARDCOUNT);
	
	// Take the measurement
	SAR1_Measure_uV(&SAR_Voltage, &SAR_Voltage_SD, sampleCount);
	
	// Convert from ADC microvolts to a measurement of current in amperes (using the value of series resistance)
	currentAverage = unitConversion * (SAR_Voltage);
	currentStdDev = unitConversion * (SAR_Voltage_SD);
	
	*currentAverageIn = currentAverage;
	*currentStdDevIn = currentStdDev;
}

// === End Section: ADC Measurement ===



// === Section: Handling Compliance ===

uint8 At_Compliance() {
	Compliance_Reached = 0;
	
	float current = 0;
	float currentSD = 0;
	
	Measure_Drain_Current(&current, &currentSD, 3);
	
	if (abs(current) > COMPLIANCE_CURRENT_LIMIT) {
		Compliance_Reached = 1;
		return 1;
	}
	
	return 0;
}

void Handle_Compliance_Breach() {
	uint8 istart = VDAC_Vds_Data;
	uint8 istop = VDAC_Ref_Data;
	int8 increment = 1;
	if (istart < istop) increment = -1;
	
	for (uint8 i = istart; i != istop; i += increment) {
		VDAC_Vds_SetValue(i);
		
		if (!At_Compliance()) return;
	}
	
	istart = VDAC_Vgs_Data;
	istop = VDAC_Ref_Data;
	increment = 1;
	if (istart < istop) increment = -1;
	
	for (uint8 i = istart; i != istop; i += increment) {
		VDAC_Vgs_SetValue(i);
		
		if (!At_Compliance()) return;
	}
	
	// To do: Should disconnect something or take some other action at this point since still at compliance
}

// === End Section: Handling Compliance ===



// === Section: DAC Getting and Setting (with unit conversions) ===

// GET: Get the reference voltage in volts
float Get_Ref() {
	float result = DAC_VOLTAGE_MAXIMUM/255.0*VDAC_Ref_Data;
	if (VDAC_Ref_CR0 & (VDAC_Ref_RANGE_1V & VDAC_Ref_RANGE_MASK)) {
		result = 1.020/255.0*VDAC_Ref_Data;
	}
	return result;
}

// GET: Get Vgs in volts
float Get_Vgs() {
	float result = DAC_VOLTAGE_MAXIMUM/255.0*VDAC_Vgs_Data - Get_Ref();
	if (VDAC_Vgs_CR0 & (VDAC_Vgs_RANGE_1V & VDAC_Vgs_RANGE_MASK)) {
		result = 1.020/255.0*VDAC_Vgs_Data - Get_Ref();
	}
	return result;
}

// GET: Get Vds in volts
float Get_Vds() {
	float result = DAC_VOLTAGE_MAXIMUM/255.0*VDAC_Vds_Data - Get_Ref();
	if (VDAC_Vds_CR0 & (VDAC_Vds_RANGE_1V & VDAC_Vds_RANGE_MASK)) {
		result = 1.020/255.0*VDAC_Vds_Data - Get_Ref();
	}
	return result;
}

// RAW: Set the raw value of Vds
void Set_Vds_Raw(uint8 value) {
	Vds_Index_Goal_Relative = (int16)value - (int16)VDAC_Ref_Data;
	
	uint8 istart = VDAC_Vds_Data;
	int8 increment = 1;
	if (istart > value) increment = -1;
	
	// Ramp up to the Vds value
	for (uint8 i = istart; i != value; i += increment) {
		VDAC_Vds_SetValue(i);
	}
	
	// Now that ramp is done, set the Vds value
	VDAC_Vds_SetValue(value);
}

// RAW: Set the raw value of Vgs
void Set_Vgs_Raw(uint8 value) {
	Vgs_Index_Goal_Relative = (int16)value - (int16)VDAC_Ref_Data;
	
	int8 increment = 1;
	uint8 istart = VDAC_Vgs_Data;
	if (value < istart) increment = -1;
	
	// Ramp up to the Vgs value
	for (uint8 i = istart; i != value; i += increment) {
		VDAC_Vgs_SetValue(i);
	}
	
	// Now that ramp is done, set the Vgs value
	VDAC_Vgs_SetValue(value);
}

// RAW: Set the raw value of the reference voltage
void Set_Ref_Raw(uint8 value) {
	if (value < 6) value = 6;
	
	int8 increment = 1;
	if (value < VDAC_Ref_Data) increment = -1;
	
	// Ramp up to the Reference value
	for (uint8 i = VDAC_Ref_Data; i != value; i += increment) {		
		int16 new_Vgs = (int16)i + Vgs_Index_Goal_Relative;
		int16 new_Vds = (int16)i + Vds_Index_Goal_Relative;
		
		if (new_Vgs > 255) new_Vgs = 255;
		if (new_Vds > 255) new_Vds = 255;
		
		if (new_Vgs < 0) new_Vgs = 0;
		if (new_Vds < 0) new_Vds = 0;
		
		VDAC_Ref_SetValue(i);
		VDAC_Vgs_SetValue(new_Vgs);
		VDAC_Vds_SetValue(new_Vds);
	}
	
	// Now that ramp is done, set the Reference value
	int16 new_Vgs = (int16)value + Vgs_Index_Goal_Relative;
	int16 new_Vds = (int16)value + Vds_Index_Goal_Relative;
	
	if (new_Vgs > 255) new_Vgs = 255;
	if (new_Vds > 255) new_Vds = 255;
	
	if (new_Vgs < 0) new_Vgs = 0;
	if (new_Vds < 0) new_Vds = 0;
	
	VDAC_Ref_SetValue(value);
	VDAC_Vgs_SetValue(new_Vgs);
	VDAC_Vds_SetValue(new_Vds);
}

// RELATIVE: Set Vds and move the reference voltage if necessary
void Set_Vds_Rel(int16 value) {
	if (value > 255) value = 255;
	if (value < -255) value = -255;
	
	int16 absolute = (int16)value + (int16)VDAC_Ref_Data;
	
	if (absolute > 255) {
		Set_Ref_Raw(VDAC_Ref_Data - (absolute - 255));
		Set_Vds_Raw(255);
	} else if (absolute < 0) {
		Set_Ref_Raw(VDAC_Ref_Data - absolute);
		Set_Vds_Raw(0);
	} else {
		Set_Vds_Raw(absolute);
	}
	
	Vds_Index_Goal_Relative = (int16)value;
	
	//Now that we have set our DAC, try to put back the reference in a nice place
	uint16 Vds_minRefRaw = Vds_Index_Goal_Relative > 0? 0: -Vds_Index_Goal_Relative;
	uint16 Vds_maxRefRaw = Vds_Index_Goal_Relative > 0? 255 - Vds_Index_Goal_Relative: 255;
	uint16 Vgs_minRefRaw = Vgs_Index_Goal_Relative > 0? 0: -Vgs_Index_Goal_Relative;
	uint16 Vgs_maxRefRaw = Vgs_Index_Goal_Relative > 0? 255 - Vgs_Index_Goal_Relative: 255;
	
	//Preferred reference voltage is in the middle of its range
	uint16 newRefRaw = 128;
	
	if (newRefRaw < Vgs_minRefRaw) newRefRaw = Vgs_minRefRaw;
	if (newRefRaw > Vgs_maxRefRaw) newRefRaw = Vgs_maxRefRaw;
	if (newRefRaw < Vds_minRefRaw) newRefRaw = Vds_minRefRaw;
	if (newRefRaw > Vds_maxRefRaw) newRefRaw = Vds_maxRefRaw;
	
	Set_Ref_Raw(newRefRaw);
}

// RELATIVE: Set Vgs and move the reference voltage if necessary
void Set_Vgs_Rel(int16 value) {
	if (value > 255) value = 255;
	if (value < -255) value = -255;
	
	int16 absolute = (int16)value + (int16)VDAC_Ref_Data;
	
	if (absolute > 255) {
		Set_Ref_Raw(VDAC_Ref_Data - (absolute - 255));
		Set_Vgs_Raw(255);
	} else if (absolute < 0) {
		Set_Ref_Raw(VDAC_Ref_Data - absolute);
		Set_Vgs_Raw(0);
	} else {
		Set_Vgs_Raw(absolute);
	}
	
	Vgs_Index_Goal_Relative = (int16)value;
	
	//Now that we have set our DAC, try to put back the reference in a nice place
	uint16 Vds_minRefRaw = Vds_Index_Goal_Relative > 0? 0: -Vds_Index_Goal_Relative;
	uint16 Vds_maxRefRaw = Vds_Index_Goal_Relative > 0? 255 - Vds_Index_Goal_Relative: 255;
	uint16 Vgs_minRefRaw = Vgs_Index_Goal_Relative > 0? 0: -Vgs_Index_Goal_Relative;
	uint16 Vgs_maxRefRaw = Vgs_Index_Goal_Relative > 0? 255 - Vgs_Index_Goal_Relative: 255;
	
	//Preferred reference voltage is in the middle of its range
	uint16 newRefRaw = 128;
	
	if (newRefRaw < Vds_minRefRaw) newRefRaw = Vds_minRefRaw;
	if (newRefRaw > Vds_maxRefRaw) newRefRaw = Vds_maxRefRaw;
	if (newRefRaw < Vgs_minRefRaw) newRefRaw = Vgs_minRefRaw;
	if (newRefRaw > Vgs_maxRefRaw) newRefRaw = Vgs_maxRefRaw;
	
	Set_Ref_Raw(newRefRaw);
}

// VOLTS: Set Vgs by volts (good for readability, but requires passing floats)
void Set_Vgs(float voltage) {
	Set_Vgs_Rel((voltage/DAC_VOLTAGE_MAXIMUM)*255.0);
}

// VOLTS: Set Vds by volts (good for readability, but requires passing floats)
void Set_Vds(float voltage) {
	Set_Vds_Rel((voltage/DAC_VOLTAGE_MAXIMUM)*255.0);
}

// MILLIVOLTS: Set Vgs by mV (good for passing in integers instead of floats)
void Set_Vgs_mV(float mV) {
	Set_Vgs(mV/1000.0);
}

// MILLIVOLTS: Set Vds by mV (good for passing in integers instead of floats)
void Set_Vds_mV(float mV) {
	Set_Vds(mV/1000.0);
}

// ZERO: Set Vgs and Vds to 0
void Zero_All_DACs() {
	Set_Vds_Rel(0);
	Set_Vgs_Rel(0);
}

// === End Section: DAC Getting and Setting (with unit conversions) ===



// === Section: Calibration ===

void Calibrate_ADC_Offset(uint32 sampleCount) {
	sprintf(TransmitBuffer, "Calibrating ADC Offsets...\r\n");
	sendTransmitBuffer();
	
	uint8 current_range_resistor = TIA_Selected_Resistor;
	
	// Zero the DACs
	Set_Vds(0);
	Set_Vgs(0);
	
	// Voltage and its standard deviation (in uV)
	int32 voltage = 0;
	int32 voltageSD = 0;
	
	// Measure ADC offset voltages
	for (uint8 i = 0; i < TIA_INTERNAL_RESISTOR_COUNT + AMUX_EXTERNAL_RESISTOR_COUNT; i++) {
		if(TIA_Resistor_Enabled[i]) {
			TIA_Set_Resistor(i);
			// Take a few measurements to discard any problems with the initial measurements
			ADC_Measure_uV(&voltage, &voltageSD, 3);
			ADC_Measure_uV(&voltage, &voltageSD, sampleCount);
			TIA_Offsets_uV[i] = -voltage;
			sprintf(TransmitBuffer, "Offset %e Ohm: %li uV.\r\n", TIA_Resistor_Values[i], -voltage);
			sendTransmitBuffer();
		}
	}
	
	// Reset TIA Resistor
	TIA_Set_Resistor(current_range_resistor);
}

void Zero_ADC_Offset() {
	for (uint8 i = 0; i < TIA_INTERNAL_RESISTOR_COUNT + AMUX_EXTERNAL_RESISTOR_COUNT; i++) {
		TIA_Offsets_uV[i] = 0;
		sprintf(TransmitBuffer, "Offset %e Ohm: 0 uV.\r\n", TIA_Resistor_Values[i]);
		sendTransmitBuffer();
	}
}

// === End Section: Calibration ===



// === Section: Device Measurement ===

// SINGLE: Take a measurement of the system (Id - from delta-sigma ADC, Vgs, Vds, SAR1 ADC, SAR2 ADC)
void Measure(uint32 deltaSigmaSampleCount, uint32 SAR1_SampleCount, uint32 SAR2_SampleCount) {
	float DrainCurrentAverageAmps = 0;
	float DrainCurrentStdDevAmps = 0;
	
	float GateCurrentAverageAmps = 0;
	float GateCurrentStdDevAmps = 0;
	
	int32 SAR2_Average = 0;
	int32 SAR2_SD = 0;
	
	// Measure drain current
	if(deltaSigmaSampleCount > 0) {
		Measure_Drain_Current(&DrainCurrentAverageAmps, &DrainCurrentStdDevAmps, deltaSigmaSampleCount);
	}
	
	// Measure gate current
	if(SAR1_SampleCount > 0) {
		Measure_Gate_Current(&GateCurrentAverageAmps, &GateCurrentStdDevAmps, SAR1_SampleCount);
	}
	
	// Extra SAR (not currently being used)
	if(SAR2_SampleCount > 0) {
		SAR2_Measure_uV(&SAR2_Average, &SAR2_SD, SAR2_SampleCount);
	}
	//float SAR2 = (1e-6) * SAR2_Average;
	
	// Convert to integers for faster communication (if desired0
	//int32 microVoltGateVoltage  = Get_Vgs() * 1e6;
	//int32 microVoltDrainVoltage = Get_Vds() * 1e6;
	//int32 DrainCurrentAveragePicoAmps = DrainCurrentAverageAmps * 1e12;
	//int32 GateCurrentAveragePicoAmps  = GateCurrentAverageAmps * 1e12;
	
	// Transmit data measurement
	sprintf(TransmitBuffer, "[%e,%f,%f,%e]\r\n", DrainCurrentAverageAmps, Get_Vgs(), Get_Vds(), GateCurrentAverageAmps);
	//sprintf(TransmitBuffer, "[%e,%f,%f,%e,%e]\r\n", DrainCurrentAverageAmps, Get_Vgs(), Get_Vds(), GateCurrentAverageAmps, TIA_Resistor_Values[TIA_Selected_Resistor]);
	//sprintf(TransmitBuffer, "[%e,%ld,%ld,%e]\r\n", DrainCurrentAverageAmps, microVoltGateVoltage, microVoltDrainVoltage, GateCurrentAverageAmps);
	//sprintf(TransmitBuffer, "[%ld,%ld,%ld,%ld]\r\n", DrainCurrentAveragePicoAmps, microVoltGateVoltage, microVoltDrainVoltage, GateCurrentAveragePicoAmps);
	sendTransmitBuffer();
}

// MULTIPLE: Repeatedly take measurements of the system
void Measure_Multiple(uint32 n) {
	for (uint32 i = 0; i < n; i++) {
		Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
	}
}

// SWEEP: simplest version of a gate sweep, with options for increment and forward/reverse directions
void Indexed_Gate_Sweep(int16 Vgs_imin, int16 Vgs_imax, uint8 increment, uint8 direction) {	
	if(!direction) {
		for (int16 Vgsi = Vgs_imin; Vgsi <= Vgs_imax; Vgsi += increment) {
			Set_Vgs_Rel(Vgsi);
			Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
		}
	} else {
		for (int16 Vgsi = Vgs_imax; Vgsi >= Vgs_imin; Vgsi -= increment) {
			Set_Vgs_Rel(Vgsi);
			Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
		}
	}
}

// SWEEP: simplest version of a drain sweep, with options for increment and forward/reverse directions
void Indexed_Drain_Sweep(int16 Vds_imin, int16 Vds_imax, uint8 increment, uint8 direction) {	
	if(!direction) {
		for (int16 Vdsi = Vds_imin; Vdsi <= Vds_imax; Vdsi += increment) {
			Set_Vds_Rel(Vdsi);
			Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
		}
	} else {
		for (int16 Vdsi = Vds_imax; Vdsi >= Vds_imin; Vdsi -= increment) {
			Set_Vds_Rel(Vdsi);
			Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
		}
	}
}

// SWEEP: gate sweep with inputs for voltage start/end and # of steps
void Voltage_Gate_Sweep(float Vgs_start, float Vgs_end, uint16 steps) {	
	if(steps < 2) steps = 2;
	float increment = (Vgs_end - Vgs_start)/((float)(steps - 1));
	float gateVoltage = Vgs_start;

	for (uint16 i = 1; i <= steps; i++) {
		Set_Vgs(gateVoltage);
		Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
		gateVoltage += increment;
	}
}

// SWEEP: drain sweep with inputs for voltage start/end and # of steps
void Voltage_Drain_Sweep(float Vds_start, float Vds_end, uint16 steps) {	
	if(steps < 2) steps = 2;
	float increment = (Vds_end - Vds_start)/((float)(steps - 1));
	float drainVoltage = Vds_start;

	for (uint16 i = 1; i <= steps; i++) {
		Set_Vds(drainVoltage);
		Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
		drainVoltage += increment;
	}
}

// SWEEP: Measure a gate sweep (with options for the voltage window and # of steps)
void Measure_Gate_Sweep(float Vgs_start, float Vgs_end, uint16 steps, uint8 loop) {
	// Ramp to initial Vgs
	Set_Vgs(Vgs_start);
	
	// Forward and (optional) reverse sweep
	Voltage_Gate_Sweep(Vgs_start, Vgs_end, steps);
	if(loop) {
		Voltage_Gate_Sweep(Vgs_end, Vgs_start, steps);
	}
}

// SWEEP: Measure a drain sweep (with options for the voltage window and # of steps)
void Measure_Drain_Sweep(float Vds_start, float Vds_end, uint16 steps, uint8 loop) {
	// Ramp to initial Vds
	Set_Vds(Vds_start);
	
	// Forward and (optional) reverse sweep
	Voltage_Drain_Sweep(Vds_start, Vds_end, steps);
	if(loop) {
		Voltage_Drain_Sweep(Vds_end, Vds_start, steps);
	}
}

// SWEEP: Measure gate sweep (with the full possible range of Vgs)
void Measure_Full_Gate_Sweep(uint8 increment, uint8 wide, uint8 loop) {	
	int16 imax = 255;
	int16 imin = -255;
	
	// If not a wide sweep, restrict Vgs to the acceptable range that will not override Vds
	if(!wide) {
		if(Vds_Index_Goal_Relative > 0) {
			imin = imin + Vds_Index_Goal_Relative;
		} else {
			imax = imax + Vds_Index_Goal_Relative;
		}
	} 
	
	// Put the reference as positive as possible (so we can start at very negative Vgs)
	Set_Ref_Raw(imax);
	
	// Forward and (optional) reverse sweep
	Indexed_Gate_Sweep(imin, imax, increment, 0);
	if (loop) {
		Indexed_Gate_Sweep(imin, imax, increment, 1);
	}
	
	// Ground gate when done
	Set_Vgs(0);
}

// SWEEP: Measure gate sweep (with the full possible range of Vgs)
void Measure_Full_Drain_Sweep(uint8 increment, uint8 wide, uint8 loop) {	
	int16 imax = 255;
	int16 imin = -255;
	
	// If not a wide sweep, restrict Vgs to the acceptable range that will not override Vds
	if(!wide) {
		if(Vgs_Index_Goal_Relative > 0) {
			imin = imin + Vgs_Index_Goal_Relative;
		} else {
			imax = imax + Vgs_Index_Goal_Relative;
		}
	} 
	
	// Put the reference as positive as possible (so we can start at very negative Vds)
	Set_Ref_Raw(imax);
	
	// Forward and (optional) reverse sweep
	Indexed_Drain_Sweep(imin, imax, increment, 0);
	if (loop) {
		Indexed_Drain_Sweep(imin, imax, increment, 1);
	}
	
	// Ground drain when done
	Set_Vds(0);
}

// SCAN: Do a gate sweep of the specified range of devices
void Scan_Range(uint8 startDevice, uint8 stopDevice, uint8 wide, uint8 loop) {
	if (startDevice >= CONTACT_COUNT) return;
	if (stopDevice >= CONTACT_COUNT) return;
	
	for (uint8 device = startDevice; device <= stopDevice; device++) {
		// Disconnect previous device
		Disconnect_All_Contacts_From_All_Selectors();
		
		// Connect this device
		uint8 contact1 = device;
		uint8 contact2 = device + 1;
		Connect_Contact_To_Source(contact1);
		Connect_Contact_To_Drain(contact2);
		
		// Print
		sprintf(TransmitBuffer, "\r\n Device %u - %u \r\n", contact1, contact2);
		sendTransmitBuffer();
		
		// Gate Sweep
		Measure_Full_Gate_Sweep(8, wide, loop);
		
		if (G_Break || G_Stop) break;
		while (G_Pause);
	}
}

// SCAN: Do a gate sweep of every device 
void Scan_All_Devices(uint8 wide, uint8 loop) {
	Scan_Range(1, CONTACT_COUNT, wide, loop);
}

// === End Section: Device Measurement ===



// === Section: USB/Bluetooth Communication Handler Definition ===

CY_ISR (CommunicationHandlerISR) {
	// --- USB Communication Handling ---
	if (USBUARTH_DataIsReady()) {
		
		// Put recieved characters into a temporary buffer
		char temp_buffer[USBUART_BUFFER_SIZE];
		uint8 temp_count = USBUART_GetAll((uint8*) temp_buffer);
		
		// For each recieved character, place it into the global USBUART_Receive_Buffer if it is not an end-of-line character.
		for (uint8 i = 0; i < temp_count; i++) {
			char c = temp_buffer[i];
			USBUART_Receive_Buffer[USBUART_Rx_Position] = c;
			
			// Replace EOL characters with '\0' and set "newData = 1" when we are ready for the message in USBUART_Receive_Buffer to be interpretted by the rest of our code.
			if (c == '\r' || c == '\n' || c == '!') {
				USBUART_Receive_Buffer[USBUART_Rx_Position] = 0;
				if (USBUART_Rx_Position) newData = 1;
				USBUART_Rx_Position = 0;
			} else {
				USBUART_Rx_Position++;
			}
			
			// OVERFLOW: this means the command was too long for our buffer to handle. There is no good solution, so we have to just start overwriting characters at the front of the buffer.
			if (USBUART_Rx_Position >= USBUART_BUFFER_SIZE) USBUART_Rx_Position = 0;
		}
	} else
	
	// --- Bluetooth Communication Handling ---
	if (UART_1_GetRxBufferSize()) {
		UART_Receive_Buffer[UART_Rx_Position] = UART_1_GetChar();
		
		// Replace EOL characters with '\0' and set "newData = 2" when we are ready for the message in UART_Receive_Buffer to be interpretted by the rest of our code.
		if (UART_Receive_Buffer[UART_Rx_Position] == '\r' || UART_Receive_Buffer[UART_Rx_Position] == '\n' || UART_Receive_Buffer[UART_Rx_Position] == '!') {
			UART_Receive_Buffer[UART_Rx_Position] = 0;
			if (UART_Rx_Position) newData = 2;
			UART_Rx_Position = 0;
		} else {
			UART_Rx_Position++;
		}
		
		// OVERFLOW: this means the command was too long for our buffer to handle. There is no good solution, so we have to just start overwriting characters at the front of the buffer.
		if (UART_Rx_Position >= USBUART_BUFFER_SIZE) UART_Rx_Position = 0;
	}
	
	// Quickly handle some high priority commands, and set globals as necessary.
	if (newData) {
		char* ReceiveBuffer = &USBUART_Receive_Buffer[0];
		if (newData == 2) ReceiveBuffer = &UART_Receive_Buffer[0];
		
		if (strstr(ReceiveBuffer, "stop ") == &ReceiveBuffer[0]) {
			G_Stop = 1;
			newData = 0;
		} else 
		if (strstr(ReceiveBuffer, "break ") == &ReceiveBuffer[0]) {
			G_Break = 1;
			newData = 0;
		} else 
		if (strstr(ReceiveBuffer, "pause ") == &ReceiveBuffer[0]) {
			G_Pause = 1;
			newData = 0;
		} else 
		if (strstr(ReceiveBuffer, "resume ") == &ReceiveBuffer[0]) {
			G_Pause = 0;
			newData = 0;
		} else 
		if (strstr(ReceiveBuffer, "tristate ") == &ReceiveBuffer[0]) {
			// To do
			newData = 0;
		}
	}
	
	CommunicationInterrupt_ClearPending();
	CommunicationTimer_ReadStatusRegister();
}

// === End Section: USB/Bluetooth Communication Handler Definition ===



// === === Program Start Point === ===

int main(void) {
	CyGlobalIntEnable;
	
	// Setup data structures for communication to the device selectors
	Setup_Selectors();
	
	// Start USB Interface
	USBUART_Start(0u, USBUART_5V_OPERATION);
	UART_1_Start();
	
	// Start communication with device selectors
	I2C_1_Start();
	
	// Start All DACs, ADCs, TIAs, and AMUXes
	VDAC_Vds_Start();
	VDAC_Vgs_Start();
	VDAC_Ref_Start();
	TIA_1_Start();
	AMux_1_Start();
	ADC_DelSig_1_Start();
	#ifdef ADC_SAR_1_RETURN_STATUS //only defined if SAR_1 is enabled in the design
	ADC_SAR_1_Start();
	ADC_SAR_1_StartConvert();
	#endif
	#ifdef ADC_SAR_2_RETURN_STATUS //only defined if SAR_2 is enabled in the design
	ADC_SAR_2_Start();
	ADC_SAR_2_StartConvert();
	#endif
	
	// Start the op-amp buffers
	Opamp_1_Start();
	Opamp_2_Start();
	Opamp_3_Start();
	Opamp_4_Start();
	
	// Tell the ADC to begin sampling continuously
	ADC_DelSig_1_StartConvert();
	
	// Delay to give all components time to activate
	CyDelay(1000);
	
	// === All components now active ===
	
	
	
	// Connect SS1 and DD1 to the analog MUXes of any device selector they are routed to
	Connect_Selectors_To_Primary_Signal_Channel();
	
	// Calibrate Delta-Sigma ADC and set initial current range
	Calibrate_ADC_Offset(ADC_CALIBRATION_SAMPLECOUNT);
	TIA_Set_Resistor(TIA_Selected_Resistor);
	
	// Take one test measurement to make sure ADCs are fully activated
	sprintf(TransmitBuffer, "Making test measurement... \r\n");
	sendTransmitBuffer();
	Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
	
	// === All components now ready ===
	
	
	
	// Print starting message
	sprintf(TransmitBuffer, "Startup complete. System is ready. \r\n");
	sendTransmitBuffer();
	
	// Prepare to receive commands from the host
	newData = 0;
	UART_Rx_Position = 0;
	USBUART_Rx_Position = 0;
	
	CommunicationTimer_Start();
	
	// Activate USB/Bluetooth communication interrupt handler
	CommunicationInterrupt_StartEx(CommunicationHandlerISR);
	
	// === Ready to receive commands ===
	
	
	
	while (1) {
		G_Stop = 0;
		G_Break = 0;
		
		if (newData) { // newData == 1 if using USB communication, newData == 2 if using bluetooth
			char* ReceiveBuffer = &USBUART_Receive_Buffer[0];
			if (newData == 2) ReceiveBuffer = &UART_Receive_Buffer[0];
			
			newData = 0;
			
			// === Section: Receiving and Executing Commands ===
			if (strstr(ReceiveBuffer, "connect ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 contact = strtol(location, &location, 10);
				uint8 selector_index = strtol(location, &location, 10);
				Connect_Contact_To_Selector(contact, selector_index);
				
				sprintf(TransmitBuffer, "# Connected contact %u to selector %u.\r\n", contact, selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-channel ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 channel = strtol(location, &location, 10);
				uint8 selector_index = strtol(location, &location, 10);
				Connect_Channel_On_Selector(channel, selector_index);
				
				sprintf(TransmitBuffer, "# Connected channel %u to selector %u.\r\n", channel, selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 contact = strtol(location, &location, 10);
				uint8 selector_index = strtol(location, &location, 10);
				Disconnect_Contact_From_Selector(contact, selector_index);
				
				sprintf(TransmitBuffer, "# Disonnected contact %u from selector %u.\r\n", contact, selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-channel ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 channel = strtol(location, &location, 10);
				uint8 selector_index = strtol(location, &location, 10);
				Disconnect_Channel_On_Selector(channel, selector_index);
				
				sprintf(TransmitBuffer, "# Disconnected channel %u from selector %u.\r\n", channel, selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-contact-from-all ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 contact = strtol(location, &location, 10);
				Disconnect_Contact_From_All_Selectors(contact);
				
				sprintf(TransmitBuffer, "# Disconnected contact %u from all selectors.\r\n", contact);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-all-from-selector ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 selector_index = strtol(location, &location, 10);
				Disconnect_All_Contacts_From_Selector(selector_index);
				
				sprintf(TransmitBuffer, "# Disconnected all contacts from selector %u.\r\n", selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-all-from-all ") == &ReceiveBuffer[0]) {
				Disconnect_All_Contacts_From_All_Selectors();
				
				sprintf(TransmitBuffer, "# Disconnected all contacts from all selectors.\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-selector ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 selector_index = strtol(location, &location, 10);
				Connect_Selector_Signal_Channel1(selector_index);
				
				sprintf(TransmitBuffer, "# Connected selector %u to primary signal channel.\r\n", selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-selector-secondary ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 selector_index = strtol(location, &location, 10);
				Connect_Selector_Signal_Channel2(selector_index);
				
				sprintf(TransmitBuffer, "# Connected selector %u to secondary signal channel.\r\n", selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-selector ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 selector_index = strtol(location, &location, 10);
				Disconnect_Selector_Signal_Channel1(selector_index);
				Disconnect_Selector_Signal_Channel2(selector_index);
				
				sprintf(TransmitBuffer, "# Disconnected selector %u from signal channels.\r\n", selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-all-selectors ") == &ReceiveBuffer[0]) {
				Connect_Selectors_To_Primary_Signal_Channel();
				
				sprintf(TransmitBuffer, "# Connected all selectors to primary signal channels.\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-all-selectors-secondary ") == &ReceiveBuffer[0]) {
				Connect_Selectors_To_Secondary_Signal_Channel();
				
				sprintf(TransmitBuffer, "# Connected all selectors to secondary signal channels.\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-all-selectors ") == &ReceiveBuffer[0]) {
				Disconnect_Selectors();
				
				sprintf(TransmitBuffer, "# Disconnected all selectors from signal channels.\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "switch-all-selectors-to-signal ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 signal_channel = strtol(location, &location, 10);
				
				Switch_Selectors_To_Signal_Channel(signal_channel);
				
				sprintf(TransmitBuffer, "# Connected all selectors to signal channel %u.\r\n", signal_channel);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-source-to ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 contact = strtol(location, &location, 10);
				uint8 selector_index = Connect_Contact_To_Source(contact);
				
				sprintf(TransmitBuffer, "# Connected contact %u to selector %u (source).\r\n", contact, selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-drain-to ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 contact = strtol(location, &location, 10);
				uint8 selector_index = Connect_Contact_To_Drain(contact);
				
				sprintf(TransmitBuffer, "# Connected contact %u to selector %u (drain).\r\n", contact, selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-device ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 contact1 = strtol(location, &location, 10);
				uint8 contact2 = strtol(location, &location, 10);
				uint8 selector1 = Connect_Contact_To_Source(contact1);
				uint8 selector2 = Connect_Contact_To_Drain(contact2);
				
				sprintf(TransmitBuffer, "# Connected device %u-%u to selectors %u (S) and %u (D).\r\n", contact1, contact2, selector1, selector2);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vgs-raw ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 vgsi = strtol(location, &location, 10);
				Set_Vgs_Raw(vgsi);
				
				sprintf(TransmitBuffer, "# Vgs raw set to %u.\r\n", vgsi);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vds-raw ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 vdsi = strtol(location, &location, 10);
				Set_Vds_Raw(vdsi);
				
				sprintf(TransmitBuffer, "# Vds raw set to %u.\r\n", vdsi);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vgs-rel ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				int16 vgsi = strtol(location, &location, 10);
				Set_Vgs_Rel(vgsi);
				
				sprintf(TransmitBuffer, "# Vgs relative set to %d.\r\n", vgsi);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vds-rel ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				int16 vdsi = strtol(location, &location, 10);
				Set_Vds_Rel(vdsi);
				
				sprintf(TransmitBuffer, "# Vds relative set to %d.\r\n", vdsi);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vgs ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float Vgs = (float)(strtod(location, &location));
				Set_Vgs(Vgs);
				
				sprintf(TransmitBuffer, "# Vgs set to %f V.\r\n", Vgs);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vds ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float Vds = (float)(strtod(location, &location));
				Set_Vds(Vds);
				
				sprintf(TransmitBuffer, "# Vds set to %f V.\r\n", Vds);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vgs-mv ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float Vgs_mV = (float)(strtol(location, &location, 10));
				Set_Vgs_mV(Vgs_mV);
				
				sprintf(TransmitBuffer, "# Vgs set to %f mV.\r\n", Vgs_mV);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vds-mv ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float Vds_mV = (float)(strtol(location, &location, 10));
				Set_Vds_mV(Vds_mV);
				
				sprintf(TransmitBuffer, "# Vds set to %f mV.\r\n", Vds_mV);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "calibrate-adc-offset ") == &ReceiveBuffer[0]) {
				Calibrate_ADC_Offset(ADC_CALIBRATION_SAMPLECOUNT);
				
				sprintf(TransmitBuffer, "# Calibrated ADC Offsets.\r\n");
				sendTransmitBuffer();
			} else
			if (strstr(ReceiveBuffer, "zero-adc-offset ") == &ReceiveBuffer[0]) {
				Zero_ADC_Offset();
				
				sprintf(TransmitBuffer, "# Set ADC Offsets to zero.\r\n");
				sendTransmitBuffer();
			} else
			if (strstr(ReceiveBuffer, "measure ") == &ReceiveBuffer[0]) {
				Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
			} else 
			if (strstr(ReceiveBuffer, "measure-multiple ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint32 n = strtol(location, &location, 10);
				Measure_Multiple(n);
			} else 
			if (strstr(ReceiveBuffer, "gate-sweep ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float Vgs_start = (float)(strtod(location, &location));
				float Vgs_end = (float)(strtod(location, &location));
				uint16 steps = (strtol(location, &location, 10));
				Measure_Gate_Sweep(Vgs_start, Vgs_end, steps, 0);
			} else 
			if (strstr(ReceiveBuffer, "gate-sweep-loop ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float Vgs_start = (float)(strtod(location, &location));
				float Vgs_end = (float)(strtod(location, &location));
				uint16 steps = (strtol(location, &location, 10));
				Measure_Gate_Sweep(Vgs_start, Vgs_end, steps, 1);
			} else 
			if (strstr(ReceiveBuffer, "full-gate-sweep ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 Vgs_increment = (strtol(location, &location, 10));
				Measure_Full_Gate_Sweep(Vgs_increment, 0, 0);
			} else 
			if (strstr(ReceiveBuffer, "full-gate-sweep-loop ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 Vgs_increment = (strtol(location, &location, 10));
				Measure_Full_Gate_Sweep(Vgs_increment, 0, 1);
			} else 
			if (strstr(ReceiveBuffer, "full-wide-gate-sweep ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 Vgs_increment = (strtol(location, &location, 10));
				Measure_Full_Gate_Sweep(Vgs_increment, 1, 0);
			} else 
			if (strstr(ReceiveBuffer, "full-wide-gate-sweep-loop ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 Vgs_increment = (strtol(location, &location, 10));
				Measure_Full_Gate_Sweep(Vgs_increment, 1, 1);
			} else 
			if (strstr(ReceiveBuffer, "drain-sweep ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float Vds_start = (float)(strtod(location, &location));
				float Vds_end = (float)(strtod(location, &location));
				uint16 steps = (strtol(location, &location, 10));
				Measure_Drain_Sweep(Vds_start, Vds_end, steps, 0);
			} else 
			if (strstr(ReceiveBuffer, "drain-sweep-loop ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float Vds_start = (float)(strtod(location, &location));
				float Vds_end = (float)(strtod(location, &location));
				uint16 steps = (strtol(location, &location, 10));
				Measure_Drain_Sweep(Vds_start, Vds_end, steps, 1);
			} else 
			if (strstr(ReceiveBuffer, "full-drain-sweep ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 Vds_increment = (strtol(location, &location, 10));
				Measure_Full_Drain_Sweep(Vds_increment, 0, 0);
			} else 
			if (strstr(ReceiveBuffer, "full-drain-sweep-loop ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 Vds_increment = (strtol(location, &location, 10));
				Measure_Full_Drain_Sweep(Vds_increment, 0, 1);
			} else 
			if (strstr(ReceiveBuffer, "scan-all-devices ") == &ReceiveBuffer[0]) {
				sprintf(TransmitBuffer, "\r\n# Scan of All Devices Starting.\r\n");
				sendTransmitBuffer();
				
				Scan_All_Devices(0, 0);
				
				sprintf(TransmitBuffer, "\r\n# Scan of All Devices Complete.\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "scan-range ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 startDevice = strtol(location, &location, 10);
				uint8 stopDevice = strtol(location, &location, 10);
				sprintf(TransmitBuffer, "\r\n# Scan-Range (%u to %u) Starting.\r\n", startDevice, stopDevice);
				sendTransmitBuffer();
				
				Scan_Range(startDevice, stopDevice, 0, 0);
				
				sprintf(TransmitBuffer, "\r\n# Scan-Range (%u to %u) Complete.\r\n", startDevice, stopDevice);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "scan-range-loop ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 startDevice = strtol(location, &location, 10);
				uint8 stopDevice = strtol(location, &location, 10);
				sprintf(TransmitBuffer, "\r\n# Scan-Range-Loop (%u to %u) Starting.\r\n", startDevice, stopDevice);
				sendTransmitBuffer();
				
				Scan_Range(startDevice, stopDevice, 0, 1);
				
				sprintf(TransmitBuffer, "\r\n# Scan-Range-Loop (%u to %u) Complete.\r\n", startDevice, stopDevice);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "enable-uart-sending ") == &ReceiveBuffer[0]) {
				uartSendingEnabled = true;
				
				sprintf(TransmitBuffer, "# Enabled UART Sending.\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "enable-usbu-sending ") == &ReceiveBuffer[0]) {
				usbuSendingEnabled = true;
				
				sprintf(TransmitBuffer, "# Enabled USBU Sending.\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disable-uart-sending ") == &ReceiveBuffer[0]) {
				uartSendingEnabled = false;
				
				sprintf(TransmitBuffer, "# Disabled UART Sending.\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disable-usbu-sending ") == &ReceiveBuffer[0]) {
				usbuSendingEnabled = false;
				
				sprintf(TransmitBuffer, "# Disabled USBU Sending.\r\n");
				sendTransmitBuffer();
			} else {
				sprintf(TransmitBuffer, "! Unidentified command: |%s|\r\n", ReceiveBuffer);
				sendTransmitBuffer();
			}
			// === End Section: Receiving and Executing Commands ===
		}
		
		// Loop continuously
	}
	
	// End of main method
}
