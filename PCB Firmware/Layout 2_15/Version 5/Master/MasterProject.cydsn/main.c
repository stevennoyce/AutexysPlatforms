// === Imports ===
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "project.h"
#include "USBUART_Helpers.h"



// === Constants ===
#define SELECTOR_COUNT (4u)
#define CONTACT_COUNT (64u)
#define CHANNEL_COUNT (34u)

#define CONTACT_CONNECT_CODE (0xC0u)
#define CONTACT_DISCONNECT_CODE (0xD1u)
#define SELECTOR1_I2C_BUS_ADDRESS (0x66)
#define SELECTOR2_I2C_BUS_ADDRESS (0x11)
#define SELECTOR3_I2C_BUS_ADDRESS (0x44)
#define SELECTOR4_I2C_BUS_ADDRESS (0x22)
#define SELECTOR_SIGNAL_CHANNEL (33)

#define COMPLIANCE_CURRENT_LIMIT (10e-6)

// De-noising Parameters
#define ADC_MEASUREMENT_CHUNKSIZE (5)
#define SAR_MEASUREMENT_CHUNKSIZE (5)
#define CURRENT_MEASUREMENT_DISCARDCOUNT (3)
#define AUTO_RANGE_SAMPLECOUNT (3)
#define AUTO_RANGE_DISCARDCOUNT (5)
#define ADC_CALIBRATION_SAMPLECOUNT (300)

// Primary Measurement Parameters
#define DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT (100)
#define GATE_CURRENT_MEASUREMENT_SAMPLECOUNT (50)
#define ADC_INCREASE_RANGE_THRESHOLD (870400)
#define ADC_DECREASE_RANGE_THRESHOLD (10240)



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



// === Variables === 
struct Selector_I2C_Struct selectors[SELECTOR_COUNT];
char TransmitBuffer[USBUART_BUFFER_SIZE];

volatile uint8 newData = 0;
volatile uint8 G_Stop = 0;
volatile uint8 G_Break = 0;
volatile uint8 G_Pause = 0;

volatile char UART_Receive_Buffer[USBUART_BUFFER_SIZE];
volatile uint8 UART_Rx_Position;

volatile char USBUART_Receive_Buffer[USBUART_BUFFER_SIZE];
volatile uint8 USBUART_Rx_Position;

// TIA1 Properties
enum TIA_resistor {R20K, R30K, R40K, R80K, R120K, R250K, R500K, R1000K};
uint8 TIA1_Selected_Resistor = R20K;
uint8 TIA1_Resistor_Codes[8] = {TIA_1_RES_FEEDBACK_20K, TIA_1_RES_FEEDBACK_30K, TIA_1_RES_FEEDBACK_40K, TIA_1_RES_FEEDBACK_80K, TIA_1_RES_FEEDBACK_120K, TIA_1_RES_FEEDBACK_250K, TIA_1_RES_FEEDBACK_500K, TIA_1_RES_FEEDBACK_1000K};
float TIA1_Resistor_Values[8] = {20e3, 30e3, 40e3, 80e3, 120e3, 250e3, 500e3, 1e6};
int32 TIA1_Offsets_uV[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// Setpoints for VGS and VDS (in the 16-bit format that the DACs use)
int16 Vgs_Index_Goal_Relative;
int16 Vds_Index_Goal_Relative;

// Alert that maximum current limit has been exceeded
uint8 Compliance_Reached;

bool uartSendingEnabled = true;
bool usbuSendingEnabled = true;




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
	
	for (uint16 i = 0; i < sizeof(selector->write.data); i++) {
		selector->write.data[i] = i + 6;
	}
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
	sprintf(TransmitBuffer, "Updating Selector %u\r\n", selector_index + 1);
	sendTransmitBuffer();
	
	// Get reference to selector communication data structure for this selector index
	struct Selector_I2C_Struct* selector = &selectors[selector_index];
	
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
	
	// I2C_1_MasterClearStatus();
	// I2C_1_MasterWriteBuf(selector->busAddress, (uint8 *) &selector->read.subAddress, 1, I2C_1_MODE_NO_STOP);
	// for (uint32 i - 0; i < 4e5; i++) {
	// 	if ((I2C_1_MasterStatus() & I2C_1_MSTAT_WR_CMPLT)) break;
	// }
	
	// I2C_1_MasterReadBuf(selector->busAddress, (uint8 *) &selector->read.data, sizeof(selector->read.data), I2C_1_MODE_REPEAT_START);
	// for (uint32 i - 0; i < 4e5; i++) {
	// 	if ((I2C_1_MasterStatus() & I2C_1_MSTAT_RD_CMPLT)) break;
	// }
	
	if (I2C_1_MasterStatus() & I2C_1_MSTAT_ERR_XFER) {
		if(I2C_1_MSTAT_ERR_ADDR_NAK) {
			sprintf(TransmitBuffer, "I2C Transfer Error! Type: NAK\r\n");
			sendTransmitBuffer();
		} else {
			sprintf(TransmitBuffer, "I2C Transfer Error! \r\n");
			sendTransmitBuffer();
		}
	}
	
	sprintf(TransmitBuffer, "Updated Selector %u\r\n", selector_index + 1);
	sendTransmitBuffer();
}

// CONNECT: Make a connection on one of the 34 channels in a device selector's analog mux
void Connect_Channel_On_Selector(uint8 channel, uint8 selector_index) {
	channel--;
	selector_index--;
	
	if ((channel >= CHANNEL_COUNT) || (selector_index >= SELECTOR_COUNT)) return;
	
	selectors[selector_index].write.data[channel] = CONTACT_CONNECT_CODE;
	Update_Selector(selector_index);
}

// DISCONNECT: Break a connection on one of the 34 channels in a device selector's analog mux
void Disconnect_Channel_On_Selector(uint8 channel, uint8 selector_index) {
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

// CONNECT: Makes the connection on the source/drain-signal channel for a specific device selector
void Connect_Selector(uint8 selector_index) {
	switch (selector_index) {
		case 1: Connect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL, 1); break;
		case 2: Connect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL, 2); break;
		case 3: Connect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL, 3); break;
		case 4: Connect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL, 4); break;
		default: return;
	}
}

// DISCONNECT: Breaks the connection on the drain/source-signal channel for a specific device selector
void Disconnect_Selector(uint8 selector_index) {
	switch (selector_index) {
		case 1: Disconnect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL, 1); break;
		case 2: Disconnect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL, 2); break;
		case 3: Disconnect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL, 3); break;
		case 4: Disconnect_Channel_On_Selector(SELECTOR_SIGNAL_CHANNEL, 4); break;
		default: return;
	}
}

// CONNECT: Connects the source/drain-signal channels to all device selectors
void Connect_Selectors() {
	for (uint8 i = 1; i <= SELECTOR_COUNT; i++) {
		Connect_Selector(i);
	}
}

// CONNECT: Disconnects the source/drain-signal channels from all device selectors
void Disconnect_Selectors() {
	for (uint8 i = 1; i <= SELECTOR_COUNT; i++) {
		Disconnect_Selector(i);
	}
}

// === End Section: Device Selectors & Communication ===



// === Section: ADC Range Selection ===

// Change the feedback resistor used in current measurement circuit
void TIA1_Set_Resistor(uint8 resistor) {
	TIA1_Selected_Resistor = resistor;
	TIA_1_SetResFB(TIA1_Resistor_Codes[resistor]);
}

// Shift feedback resistance to allow ADC to measure larger currents
void ADC_Increase_Range() {
	switch(TIA1_Selected_Resistor) {
		case R20K: 	 break;
		//case R30K: 	 TIA1_Set_Resistor(R20K); break;
		//case R40K:	 TIA1_Set_Resistor(R30K); break;
		//case R80K:	 TIA1_Set_Resistor(R40K); break;
		//case R120K:	 TIA1_Set_Resistor(R80K); break;
		//case R250K:	 TIA1_Set_Resistor(R120K); break;
		//case R500K:	 TIA1_Set_Resistor(R250K); break;
		case R1000K: TIA1_Set_Resistor(R20K); break;
		default: return;
	}
}

// Shift feedback resistance to allow ADC to measure smaller currents
void ADC_Decrease_Range() {
	switch(TIA1_Selected_Resistor) {
		case R20K: 	 TIA1_Set_Resistor(R1000K); break;
		//case R30K: 	 TIA1_Set_Resistor(R40K); break;
		//case R40K:	 TIA1_Set_Resistor(R80K); break;
		//case R80K:	 TIA1_Set_Resistor(R120K); break;
		//case R120K:	 TIA1_Set_Resistor(R250K); break;
		//case R250K:	 TIA1_Set_Resistor(R500K); break;
		//case R500K:	 TIA1_Set_Resistor(R1000K); break;
		case R1000K: break;
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

// MEASURE: Measure Delta-Sigma ADC
void ADC_Measure_uV(int32* average, int32* standardDeviation, uint32 sampleCount) {
	int32 ADC_Result = 0;
	int32 ADC_SD = 0;
	
	// Split measurements into chunks, taking the median of each chunk and averaging those medians for the final result
	uint32 medianArraySize = ADC_MEASUREMENT_CHUNKSIZE;
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

// RANGE: take a test measurement of the delta-sigma ADC and adjust its range if necessary
void ADC_Adjust_Range(uint32 sampleCount) {
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

// MEASURE: Measure SAR1 ADC
void SAR1_Measure_uV(int32* average, int32* standardDeviation, uint32 sampleCount) {
	int32 SAR_Result = 0;
	int32 SAR_SD = 0;
	
	// Split measurements into chunks, taking the median of each chunk and averaging those medians for the final result
	uint32 medianArraySize = SAR_MEASUREMENT_CHUNKSIZE;
	if (sampleCount < medianArraySize) medianArraySize = sampleCount;
	
	uint32 medianCount = sampleCount/medianArraySize;
	int32 medianArray[medianArraySize];
	
	for (uint32 i = 1; i <= medianCount; i++) {
		for (uint32 j = 0; j < medianArraySize; j++) {
			ADC_SAR_1_StartConvert();
			while (!ADC_SAR_1_IsEndConversion(ADC_SAR_1_RETURN_STATUS));
			medianArray[j] = ADC_SAR_1_CountsTo_uVolts(ADC_SAR_1_GetResult16());
		}
		
		int32 SAR1_Result_Current = medianOfArray(medianArray, medianArraySize);
		
		SAR_SD += (float)(i-1)/(float)(i)*(SAR1_Result_Current - SAR_Result)*(SAR1_Result_Current - SAR_Result);
		SAR_Result += ((float)SAR1_Result_Current - (float)SAR_Result)/(float)i;
	}
	
	*average = SAR_Result;
	*standardDeviation = SAR_SD;
}

// MEASURE: Measure SAR2 ADC
void SAR2_Measure_uV(int32* average, int32* standardDeviation, uint32 sampleCount) {
	int32 SAR_Result = 0;
	int32 SAR_SD = 0;
	
	// Split measurements into chunks, taking the median of each chunk and averaging those medians for the final result
	uint32 medianArraySize = SAR_MEASUREMENT_CHUNKSIZE;
	if (sampleCount < medianArraySize) medianArraySize = sampleCount;
	
	uint32 medianCount = sampleCount/medianArraySize;
	int32 medianArray[medianArraySize];
	
	for (uint32 i = 1; i <= medianCount; i++) {
		for (uint32 j = 0; j < medianArraySize; j++) {
			ADC_SAR_2_StartConvert();
			while (!ADC_SAR_2_IsEndConversion(ADC_SAR_2_RETURN_STATUS));
			medianArray[j] = ADC_SAR_2_CountsTo_uVolts(ADC_SAR_2_GetResult16());
		}
		
		int32 SAR2_Result_Current = medianOfArray(medianArray, medianArraySize);
		
		SAR_SD += (float)(i-1)/(float)(i)*(SAR2_Result_Current - SAR_Result)*(SAR2_Result_Current - SAR_Result);
		SAR_Result += ((float)SAR2_Result_Current - (float)SAR_Result)/(float)i;
	}
	
	*average = SAR_Result;
	*standardDeviation = SAR_SD;
}

// MEASURE: Measure drain current
void Measure_Drain_Current(float* currentAverageIn, float* currentStdDevIn, uint32 sampleCount) {		
	// Auto-adjust the current range
	ADC_Adjust_Range(AUTO_RANGE_SAMPLECOUNT);
	
	// After auto-ranging completes, determine voltage-current conversion from the chosen feedback resistance 
	float TIA1_Feedback_R = TIA1_Resistor_Values[TIA1_Selected_Resistor];
	int32 TIA1_Offset_uV = TIA1_Offsets_uV[TIA1_Selected_Resistor];
	float unitConversion = -1.0e-6/TIA1_Feedback_R;
	//unitConversion = -1.0e-6/100e6; //override internal TIA resistor value here
	
	// Voltage and its standard deviation (in uV)
	int32 ADC_Voltage = 0;
	int32 ADC_Voltage_SD = 0;
	
	// Current and its standard deviation (in A)
	float currentAverage = 0;
	float currentStdDev = 0;
	
	// Allow for the first measurement (normally not correct) to take place
	ADC_Measure_uV(&ADC_Voltage, &ADC_Voltage_SD, CURRENT_MEASUREMENT_DISCARDCOUNT);
	
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
	int32 ADC_Voltage = 0;
	int32 ADC_Voltage_SD = 0;
	
	// Current and its standard deviation (in A)
	float currentAverage = 0;
	float currentStdDev = 0;
	
	// Take the measurement
	SAR1_Measure_uV(&ADC_Voltage, &ADC_Voltage_SD, sampleCount);
	
	// Convert from ADC microvolts to a measurement of current in amperes (using the value of series resistance)
	currentAverage = unitConversion * (ADC_Voltage);
	currentStdDev = unitConversion * (ADC_Voltage_SD);
	
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
	float result = 4.080/255.0*VDAC_Ref_Data;
	if (VDAC_Ref_CR0 & (VDAC_Ref_RANGE_1V & VDAC_Ref_RANGE_MASK)) {
		result = 1.020/255.0*VDAC_Ref_Data;
	}
	return result;
}

// GET: Get Vgs in volts
float Get_Vgs() {
	float result = 4.080/255.0*VDAC_Vgs_Data - Get_Ref();
	if (VDAC_Vgs_CR0 & (VDAC_Vgs_RANGE_1V & VDAC_Vgs_RANGE_MASK)) {
		result = 1.020/255.0*VDAC_Vgs_Data - Get_Ref();
	}
	return result;
}

// GET: Get Vds in volts
float Get_Vds() {
	float result = 4.080/255.0*VDAC_Vds_Data - Get_Ref();
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
	Set_Vgs_Rel((voltage/4.080)*255.0);
}

// VOLTS: Set Vds by volts (good for readability, but requires passing floats)
void Set_Vds(float voltage) {
	Set_Vds_Rel((voltage/4.080)*255.0);
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
	uint8 current_range_resistor = TIA1_Selected_Resistor;
	
	// Zero the DACs
	Set_Vds(0);
	Set_Vgs(0);
	
	// Voltage and its standard deviation (in uV)
	int32 voltage = 0;
	int32 voltageSD = 0;
	
	// Measure ADC offset voltages
	for (uint8 i = R20K; i <= R1000K; i++) {
		TIA1_Set_Resistor(i);
		// Take a few measurements to discard any problems with the initial measurements
		ADC_Measure_uV(&voltage, &voltageSD, 3);
		ADC_Measure_uV(&voltage, &voltageSD, sampleCount);
		TIA1_Offsets_uV[i] = -voltage;
		sprintf(TransmitBuffer, "# Offset %e Ohm: %li uV\r\n", TIA1_Resistor_Values[i], -voltage);
		sendTransmitBuffer();
	}
	
	// Reset TIA Resistor
	TIA1_Set_Resistor(current_range_resistor);
	
	sprintf(TransmitBuffer, "# Calibrated ADC Offsets\r\n");
	sendTransmitBuffer();
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
	float SAR2 = (1e-6) * SAR2_Average;
	
	sprintf(TransmitBuffer, "[%e,%f,%f,%e]\r\n", DrainCurrentAverageAmps, Get_Vgs(), Get_Vds(), GateCurrentAverageAmps);
	sendTransmitBuffer();
}

// MULTIPLE: Repeatedly take measurements of the system
void Measure_Multiple(uint32 n) {
	for (uint32 i = 0; i < n; i++) {
		Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
	}
}

// SWEEP: simplest version of a gate sweep, with options for speed and forward/reverse directions
void Indexed_Gate_Sweep(int16 Vgs_imin, int16 Vgs_imax, uint8 speed, uint8 direction) {	
	if(!direction) {
		for (int16 Vgsi = Vgs_imin; Vgsi <= Vgs_imax; Vgsi += speed) {
			Set_Vgs_Rel(Vgsi);
			Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
		}
	} else {
		for (int16 Vgsi = Vgs_imax; Vgsi >= Vgs_imin; Vgsi -= speed) {
			Set_Vgs_Rel(Vgsi);
			Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
		}
	}
}

// SWEEP: gate sweep which does unit coversions for voltage and 
void Voltage_Gate_Sweep(float Vgs_start, float Vgs_end, int16 steps) {	
	if(steps < 2) steps = 2;
	float increment = (Vgs_end - Vgs_start)/((float)(steps - 1));
	float gateVoltage = Vgs_start;

	for (int16 i = 1; i <= steps; i++) {
		Set_Vgs(gateVoltage);
		Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
		gateVoltage += increment;
	}
}

// SWEEP: Measure a gate sweep (with options for the voltage window and # of steps)
void Measure_Gate_Sweep(float Vgs_start, float Vgs_end, int16 steps, uint8 loop) {
	// Ramp to initial Vgs
	Set_Vgs(Vgs_start);
	
	// Forward and (optional) reverse sweep
	Voltage_Gate_Sweep(Vgs_start, Vgs_end, steps);
	if(loop) {
		Voltage_Gate_Sweep(Vgs_end, Vgs_start, steps);
	}
	
	// Ground gate when done
	Set_Vgs(0);
}

// SWEEP: Measure gate sweep (with the full possible range of Vgs)
void Measure_Full_Gate_Sweep(uint8 speed, uint8 wide, uint8 loop) {	
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
	
	// Begin sweep
	for (uint8 l = 0; l <= loop; l++) {
		Indexed_Gate_Sweep(imin, imax, speed, l%2);
	}
	
	// Ground gate when done
	Set_Vgs(0);
}

// SCAN: Do a gate sweep of the specified range of devices
void Scan_Range(uint8 startDevice, uint8 stopDevice, uint8 wide, uint8 loop) {
	if (startDevice >= CONTACT_COUNT) return;
	if (stopDevice >= CONTACT_COUNT) return;
	
	for (uint8 device = startDevice; device <= stopDevice; device++) {
		uint8 contact1 = device;
		uint8 contact2 = device + 1;
		
		Disconnect_All_Contacts_From_All_Selectors();
		Connect_Contact_To_Selector(contact1, 1);
		Connect_Contact_To_Selector(contact2, 2);
		
		sprintf(TransmitBuffer, "\r\n Device %u - %u \r\n", contact1, contact2);
		sendTransmitBuffer();
		
		Measure_Full_Gate_Sweep(8, wide, loop);
		
		if (G_Break || G_Stop) break;
		while (G_Pause);
	}
}

// SCAN: Do a gate sweep of every device 
void Scan_All_Devices(uint8 wide, uint8 loop) {
	for (uint8 device = 1; device <= CONTACT_COUNT/2; device++) {
		uint8 contact1 = device;
		uint8 contact2 = device + 1;
		
		Disconnect_All_Contacts_From_All_Selectors();
		Connect_Contact_To_Selector(contact1, 1);
		Connect_Contact_To_Selector(contact2, 2);
		
		sprintf(TransmitBuffer, "\r\n Device %u - %u \r\n", contact1, contact2);
		sendTransmitBuffer();
		
		Measure_Full_Gate_Sweep(8, wide, loop);
		
		if (G_Break || G_Stop) break;
		while (G_Pause);
	}
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
	
	// Start All DACs, ADCs, and TIAs
	VDAC_Vds_Start();
	VDAC_Vgs_Start();
	VDAC_Ref_Start();
	ADC_DelSig_1_Start();
	ADC_SAR_1_Start();
	ADC_SAR_2_Start();
	TIA_1_Start();
	
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
	Connect_Selectors();
	
	// Calibrate Delta-Sigma ADC and set initial current range
	Calibrate_ADC_Offset(ADC_CALIBRATION_SAMPLECOUNT);
	TIA1_Set_Resistor(TIA1_Selected_Resistor);
	
	// === All components now ready ===
	
	
	
	// Print starting message
	UART_1_PutString("\r\n# Starting\r\n");
	
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
				
				sprintf(TransmitBuffer, "# Connected %u to %u\r\n", contact, selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-c ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 channel = strtol(location, &location, 10);
				uint8 selector_index = strtol(location, &location, 10);
				Connect_Channel_On_Selector(channel, selector_index);
				
				sprintf(TransmitBuffer, "# Connected channel %u to %u\r\n", channel, selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 contact = strtol(location, &location, 10);
				uint8 selector_index = strtol(location, &location, 10);
				Disconnect_Contact_From_Selector(contact, selector_index);
				
				sprintf(TransmitBuffer, "# Disonnected %u from %u\r\n", contact, selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-c ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 channel = strtol(location, &location, 10);
				uint8 selector_index = strtol(location, &location, 10);
				Disconnect_Channel_On_Selector(channel, selector_index);
				
				sprintf(TransmitBuffer, "# Disconnected channel %u from %u\r\n", channel, selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-from-all ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 contact = strtol(location, &location, 10);
				Disconnect_Contact_From_All_Selectors(contact);
				
				sprintf(TransmitBuffer, "# Disconnected %u from all\r\n", contact);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-all-from ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 selector_index = strtol(location, &location, 10);
				Disconnect_All_Contacts_From_Selector(selector_index);
				
				sprintf(TransmitBuffer, "# Disconnected all from  %u\r\n", selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-all-from-all ") == &ReceiveBuffer[0]) {
				Disconnect_All_Contacts_From_All_Selectors();
				
				sprintf(TransmitBuffer, "# Disconnected all from all\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-selector ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 selector_index = strtol(location, &location, 10);
				Connect_Selector(selector_index);
				
				sprintf(TransmitBuffer, "# Connected selector %u\r\n", selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-selector ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 selector_index = strtol(location, &location, 10);
				Disconnect_Selector(selector_index);
				
				sprintf(TransmitBuffer, "# Disconnected selector %u\r\n", selector_index);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-intermediates ") == &ReceiveBuffer[0]) {
				Connect_Selectors();
				
				sprintf(TransmitBuffer, "# Connected all selectors\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-all-selectors ") == &ReceiveBuffer[0]) {
				Disconnect_Selectors();
				
				sprintf(TransmitBuffer, "# Disconnected all selectors\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vgs-raw ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 vgsi = strtol(location, &location, 10);
				Set_Vgs_Raw(vgsi);
				
				sprintf(TransmitBuffer, "# Vgs raw set to %u \r\n", vgsi);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vds-raw ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 vdsi = strtol(location, &location, 10);
				Set_Vds_Raw(vdsi);
				
				sprintf(TransmitBuffer, "# Vds raw set to %u \r\n", vgsi);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vgs-rel ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				int16 vgsi = strtol(location, &location, 10);
				Set_Vgs_Rel(vgsi);
				
				sprintf(TransmitBuffer, "# Vgs relative set to %d \r\n", vgsi);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vds-rel ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				int16 vdsi = strtol(location, &location, 10);
				Set_Vds_Rel(vdsi);
				
				sprintf(TransmitBuffer, "# Vds relative set to %d \r\n", vdsi);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vgs ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float Vgs = (float)(strtod(location, &location, 10));
				Set_Vgs(Vgs);
				
				sprintf(TransmitBuffer, "# Vgs set to %f V\r\n", Vgs);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vds ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float Vds = (float)(strtod(location, &location, 10));
				Set_Vds(Vds);
				
				sprintf(TransmitBuffer, "# Vds set to %f V\r\n", Vds);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vgs-mv ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float Vgs_mV = (float)(strtol(location, &location, 10));
				Set_Vgs_mV(Vgs_mV);
				
				sprintf(TransmitBuffer, "# Vgs set to %f mV\r\n", Vgs_mV);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "set-vds-mv ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float Vds_mV = (float)(strtol(location, &location, 10));
				Set_Vds_mV(Vds_mV);
				
				sprintf(TransmitBuffer, "# Vds set to %f mV\r\n", Vds_mV);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "calibrate-offset ") == &ReceiveBuffer[0]) {
				Calibrate_ADC_Offset(ADC_CALIBRATION_SAMPLECOUNT);
			} else
			if (strstr(ReceiveBuffer, "measure ") == &ReceiveBuffer[0]) {
				Measure(DRAIN_CURRENT_MEASUREMENT_SAMPLECOUNT, GATE_CURRENT_MEASUREMENT_SAMPLECOUNT, 0);
			} else 
			if (strstr(ReceiveBuffer, "measure-multiple ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint32 n = strtol(location, &location, 10);
				Measure_Multiple(n);
			} else 
			if (strstr(ReceiveBuffer, "measure-gate-sweep ") == &ReceiveBuffer[0]) {
				Measure_Gate_Sweep(-3, 3, 100, 0);
			} else 
			if (strstr(ReceiveBuffer, "measure-gate-sweep-loop ") == &ReceiveBuffer[0]) {
				Measure_Gate_Sweep(-3, 3, 100, 1);
			} else 
			if (strstr(ReceiveBuffer, "measure-full-gate-sweep ") == &ReceiveBuffer[0]) {
				Measure_Full_Gate_Sweep(8, 0, 0);
			} else 
			if (strstr(ReceiveBuffer, "measure-full-gate-sweep-loop ") == &ReceiveBuffer[0]) {
				Measure_Full_Gate_Sweep(8, 0, 1);
			} else 
			if (strstr(ReceiveBuffer, "scan ") == &ReceiveBuffer[0]) {
				sprintf(TransmitBuffer, "\r\n# Scan Starting\r\n");
				sendTransmitBuffer();
				
				Scan_All_Devices(0, 0);
				
				sprintf(TransmitBuffer, "\r\n# Scan Complete\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "scan-range ") == &ReceiveBuffer[0]) {
				sprintf(TransmitBuffer, "\r\n# Scan-Range Starting\r\n");
				sendTransmitBuffer();
				
				char* location = strstr(ReceiveBuffer, " ");
				uint8 startDevice = strtol(location, &location, 10);
				uint8 stopDevice = strtol(location, &location, 10);
				Scan_Range(startDevice, stopDevice, 0, 0);
				
				sprintf(TransmitBuffer, "\r\n# Scan Range Complete\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "scan-range-loop ") == &ReceiveBuffer[0]) {
				sprintf(TransmitBuffer, "\r\n# Scan-Range-Loop Starting\r\n");
				sendTransmitBuffer();
				
				char* location = strstr(ReceiveBuffer, " ");
				uint8 startDevice = strtol(location, &location, 10);
				uint8 stopDevice = strtol(location, &location, 10);
				Scan_Range(startDevice, stopDevice, 0, 1);
				
				sprintf(TransmitBuffer, "\r\n# Scan Range Loop Complete\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "scan-range-wide-loop ") == &ReceiveBuffer[0]) {
				sprintf(TransmitBuffer, "\r\n# Scan-Range-Wide-Loop Starting\r\n");
				sendTransmitBuffer();
				
				char* location = strstr(ReceiveBuffer, " ");
				uint8 startDevice = strtol(location, &location, 10);
				uint8 stopDevice = strtol(location, &location, 10);
				Scan_Range(startDevice, stopDevice, 1, 1);
				
				sprintf(TransmitBuffer, "\r\n# Scan Range Wide Loop Complete\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "enable-uart-sending ") == &ReceiveBuffer[0]) {
				uartSendingEnabled = true;
				
				sprintf(TransmitBuffer, "# Enabled UART Sending\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "enable-usbu-sending ") == &ReceiveBuffer[0]) {
				usbuSendingEnabled = true;
				
				sprintf(TransmitBuffer, "# Enabled USBU Sending\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disable-uart-sending ") == &ReceiveBuffer[0]) {
				uartSendingEnabled = false;
				
				sprintf(TransmitBuffer, "# Disabled UART Sending\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disable-usbu-sending ") == &ReceiveBuffer[0]) {
				usbuSendingEnabled = false;
				
				sprintf(TransmitBuffer, "# Disabled USBU Sending\r\n");
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
