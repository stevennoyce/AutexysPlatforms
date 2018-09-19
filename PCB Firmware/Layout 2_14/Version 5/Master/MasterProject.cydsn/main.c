#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "project.h"
#include "USBUART_Helpers.h"



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


// Globals ----------------------------------------------
#define SELECTOR_COUNT (4u)
#define INTERMEDIATE_COUNT (4u)
#define CONTACT_COUNT (64u)
#define CHANNEL_COUNT (34u)
#define CONTACT_CONNECT_CODE (0xC0u)
#define CONTACT_DISCONNECT_CODE (0xD1u)
#define COMPLIANCE_CURRENT_LIMIT (10e-6)

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

enum TIA_resistor {R20K, R30K, R40K, R80K, R120K, R250K, R500K, R1000K};
uint8 TIA1_Selected_Resistor = R20K;

uint8 TIA1_Resistor_Codes[8] = {TIA_1_RES_FEEDBACK_20K, TIA_1_RES_FEEDBACK_30K, TIA_1_RES_FEEDBACK_40K, TIA_1_RES_FEEDBACK_80K, TIA_1_RES_FEEDBACK_120K, TIA_1_RES_FEEDBACK_250K, TIA_1_RES_FEEDBACK_500K, TIA_1_RES_FEEDBACK_1000K};
float TIA1_Resistor_Values[8] = {20e3, 30e3, 40e3, 80e3, 120e3, 250e3, 500e3, 1e6};
int32 TIA1_Offsets_uV[8] = {0, 0, 0, 0, 0, 0, 0, 0};

uint8 Compliance_Reached;
int16 Vgs_Index_Goal_Relative;
int16 Vds_Index_Goal_Relative;

uint32 Current_Measurement_Sample_Count;

bool uartSendingEnabled = true;
bool usbuSendingEnabled = true;

void sendTransmitBuffer() {
	if (usbuSendingEnabled) USBUARTH_Send(TransmitBuffer, strlen(TransmitBuffer));
	if (uartSendingEnabled) UART_1_PutString(TransmitBuffer);
}

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

void Update_Selector(uint8 selectori) {
	sprintf(TransmitBuffer, "Updating Selector %u\r\n", selectori + 1);
	sendTransmitBuffer();
	
	struct Selector_I2C_Struct* selector = &selectors[selectori];
	
	I2C_1_MasterClearStatus();
	I2C_1_MasterWriteBuf(selector->busAddress, (uint8 *) &selector->write, sizeof(selector->write), I2C_1_MODE_COMPLETE_XFER);
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
		sprintf(TransmitBuffer, "I2C Transfer Error! ");
		sendTransmitBuffer();
		
		if (I2C_1_MasterStatus() & I2C_1_MSTAT_ERR_ADDR_NAK) {
			sprintf(TransmitBuffer, "Type: NAK");
			sendTransmitBuffer();
		}
		
		sprintf(TransmitBuffer, "\r\n");
		sendTransmitBuffer();
	}
	
	sprintf(TransmitBuffer, "Updated Selector %u\r\n", selectori + 1);
	sendTransmitBuffer();
}

void Setup_Selectors() {
	for (uint8 i = 0; i < SELECTOR_COUNT; i++) Setup_Selector_I2C_Struct(&selectors[i]);
	
	selectors[0].busAddress = 0x66;
	selectors[1].busAddress = 0x11;
	selectors[2].busAddress = 0x44;
	selectors[3].busAddress = 0x22;
}

void Connect_Channel_On_Intermediate(uint8 channel, uint8 intermediate) {
	channel--;
	intermediate--;
	
	if (channel >= CHANNEL_COUNT) return;
	if (intermediate >= INTERMEDIATE_COUNT) return;
	
	selectors[intermediate].write.data[channel] = CONTACT_CONNECT_CODE;
	Update_Selector(intermediate);
}

void Disconnect_Channel_On_Intermediate(uint8 channel, uint8 intermediate) {
	channel--;
	intermediate--;
	
	if (channel >= CHANNEL_COUNT) return;
	if (intermediate >= INTERMEDIATE_COUNT) return;
	
	selectors[intermediate].write.data[channel] = CONTACT_DISCONNECT_CODE;
	Update_Selector(intermediate);
}

void Connect_Contact_To_Intermediate(uint8 contact, uint8 intermediate) {
	contact--;
	intermediate--;
	
	if (contact >= CONTACT_COUNT) return;
	if (intermediate >= INTERMEDIATE_COUNT) return;
	
	uint8 offset = 0;
	if (intermediate >= INTERMEDIATE_COUNT/2) {
		offset = CONTACT_COUNT/2;
		if (contact < CONTACT_COUNT/2) return;
	} else {
		if (contact >= CONTACT_COUNT/2) return;
	}
	
	Connect_Channel_On_Intermediate(contact - offset + 1, intermediate + 1);
}

void Disconnect_Contact_From_Intermediate(uint8 contact, uint8 intermediate) {
	contact--;
	intermediate--;
	
	if (contact >= CONTACT_COUNT) return;
	if (intermediate >= INTERMEDIATE_COUNT) return;
	
	uint8 offset = 0;
	if (intermediate >= INTERMEDIATE_COUNT/2) {
		offset = CONTACT_COUNT/2;
		if (contact < CONTACT_COUNT/2) return;
	} else {
		if (contact >= CONTACT_COUNT/2) return;
	}
	
	selectors[intermediate].write.data[contact - offset] = CONTACT_DISCONNECT_CODE;
	
	Update_Selector(intermediate);
}

void Disconnect_Contact_From_All_Intermediates(uint8 contact) {
	contact--;
	
	for (uint8 intermediate = 0; intermediate < INTERMEDIATE_COUNT; intermediate++) {
		
		uint8 offset = 0;
		if (intermediate >= INTERMEDIATE_COUNT/2) {
			offset = CONTACT_COUNT/2;
			if (contact < CONTACT_COUNT/2) continue;
		} else {
			if (contact >= CONTACT_COUNT/2) continue;
		}
		
		selectors[intermediate].write.data[contact - offset] = CONTACT_DISCONNECT_CODE;
		
		Update_Selector(intermediate);
	}
}

void Disconnect_All_Contacts_From_All_Intermediates() {
	for (uint8 intermediate = 0; intermediate < INTERMEDIATE_COUNT; intermediate++) {
		for (uint8 contact = 0; contact < CONTACT_COUNT/2; contact++) {
			selectors[intermediate].write.data[contact] = CONTACT_DISCONNECT_CODE;
		}
		
		Update_Selector(intermediate);
	}
}

void Disconnect_All_Contacts_From_Intermediate(uint8 intermediate) {
	intermediate--;
	
	for (uint8 contact = 0; contact < CONTACT_COUNT/2; contact++) {
		selectors[intermediate].write.data[contact] = CONTACT_DISCONNECT_CODE;
	}
	
	Update_Selector(intermediate);
}

void Connect_Intermediate(uint8 intermediate) {
	switch (intermediate) {
		case 1: Connect_Channel_On_Intermediate(33, 1); break;
		case 2: Connect_Channel_On_Intermediate(33, 2); break;
		case 3: Connect_Channel_On_Intermediate(33, 3); break;
		case 4: Connect_Channel_On_Intermediate(33, 4); break;
		default: return;
	}
}

void Disconnect_Intermediate(uint8 intermediate) {
	switch (intermediate) {
		case 1: Disconnect_Channel_On_Intermediate(33, 1); break;
		case 2: Disconnect_Channel_On_Intermediate(33, 2); break;
		case 3: Disconnect_Channel_On_Intermediate(33, 3); break;
		case 4: Disconnect_Channel_On_Intermediate(33, 4); break;
		default: return;
	}
}

void Connect_Intermediates() {
	for (uint8 i = 1; i <= INTERMEDIATE_COUNT; i++) {
		Connect_Intermediate(i);
	}
}

void Disconnect_Intermediates() {
	for (uint8 i = 1; i <= INTERMEDIATE_COUNT; i++) {
		Disconnect_Intermediate(i);
	}
}

void TIA1_Set_Resistor(uint8 resistor) {
	TIA1_Selected_Resistor = resistor;
	TIA_1_SetResFB(TIA1_Resistor_Codes[resistor]);
}

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

// Measure Delta-Sigma ADC
void ADC_Measure_uV(int32* average, int32* standardDeviation, uint32 sampleCount) {
	int32 ADC_Result = 0;
	int32 ADC_SD = 0;
	
	uint32 medianArraySize = 19;
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

void ADC_Adjust_Range(uint32 sampleCount) {
	int32 ADC_Voltage = 0;
	int32 ADC_Voltage_SD = 0;
	
	ADC_Measure_uV(&ADC_Voltage, &ADC_Voltage_SD, sampleCount);
	
	if(abs(ADC_Voltage) > 870400) {
		ADC_Increase_Range();
	} else if (abs(ADC_Voltage) < 10240) {
		ADC_Decrease_Range();
	} else {
		return;
	}
	
	// If the range switches, discard a few measurements
	ADC_Measure_uV(&ADC_Voltage, &ADC_Voltage_SD, 5);
}

// Measure SAR1 ADC
void SAR1_Measure_uV(int32* average, int32* standardDeviation, uint32 sampleCount) {
	int32 SAR_Result = 0;
	int32 SAR_SD = 0;
	
	for (uint32 i = 1; i <= sampleCount; i++) {
		ADC_SAR_1_StartConvert();
		while (!ADC_SAR_1_IsEndConversion(ADC_SAR_1_RETURN_STATUS));
		
		int32 SAR1_Result_Current = ADC_SAR_1_CountsTo_uVolts(ADC_SAR_1_GetResult16());
		
		SAR_SD += (float)(i-1)/(float)(i)*(SAR1_Result_Current - SAR_Result)*(SAR1_Result_Current - SAR_Result);
		SAR_Result += ((float)SAR1_Result_Current - (float)SAR_Result)/(float)i;
	}
	
	*average = SAR_Result;
	*standardDeviation = SAR_SD;
}

// Measure SAR2 ADC
void SAR2_Measure_uV(int32* average, int32* standardDeviation, uint32 sampleCount) {
	int32 SAR_Result = 0;
	int32 SAR_SD = 0;
	
	for (uint32 i = 1; i <= sampleCount; i++) {
		ADC_SAR_2_StartConvert();
		while (!ADC_SAR_2_IsEndConversion(ADC_SAR_2_RETURN_STATUS));
		
		int32 SAR2_Result_Current = ADC_SAR_2_CountsTo_uVolts(ADC_SAR_2_GetResult16());
		
		SAR_SD += (float)(i-1)/(float)(i)*(SAR2_Result_Current - SAR_Result)*(SAR2_Result_Current - SAR_Result);
		SAR_Result += ((float)SAR2_Result_Current - (float)SAR_Result)/(float)i;
	}
	
	*average = SAR_Result;
	*standardDeviation = SAR_SD;
}

// Measure current and compare it to some compliance value
void Measure_Current_Vss(float* currentAverageIn, float* currentStdDevIn, uint32 sampleCount, uint8 checkCompliance) {
	Compliance_Reached = 0;
		
	// Auto-adjust the current range
	ADC_Adjust_Range(3);
	
	// Voltage and its standard deviation (in uV)
	int32 ADC_Voltage = 0;
	int32 ADC_Voltage_SD = 0;
	
	// Current and its standard deviation (in A)
	float currentNow = 0;
	float currentAverage = 0;
	float currentStdDev = 0;
	
	float TIA1_Feedback_R = TIA1_Resistor_Values[TIA1_Selected_Resistor];
	int32 TIA1_Offset_uV = TIA1_Offsets_uV[TIA1_Selected_Resistor];
	float unitConversion = -1.0e-6/TIA1_Feedback_R;
	
	// Allow for the first measurement (normally not correct) to take place
	ADC_Measure_uV(&ADC_Voltage, &ADC_Voltage_SD, 3);
	
	// Now take the real measurement
	if (checkCompliance) {
		for (uint32 i = 1; i <= sampleCount; i++) {
			ADC_Measure_uV(&ADC_Voltage, &ADC_Voltage_SD, 1);
			
			currentNow = unitConversion * (ADC_Voltage + TIA1_Offset_uV);
			
			currentStdDev += ((float)i-1.0)/(float)(i)*(currentNow - currentAverage)*(currentNow - currentAverage);
			currentAverage += (currentNow - currentAverage)/(float)i;
			
			if (abs(currentNow) > COMPLIANCE_CURRENT_LIMIT) {
				Compliance_Reached += 1;
				break;
			} else {
				Compliance_Reached = 0;
			}
		}
	} else {
		ADC_Measure_uV(&ADC_Voltage, &ADC_Voltage_SD, sampleCount);
			
		currentAverage = unitConversion * (ADC_Voltage + TIA1_Offset_uV);
		currentStdDev = unitConversion * (ADC_Voltage_SD + TIA1_Offset_uV);
	}
	
	*currentAverageIn = currentAverage;
	*currentStdDevIn = currentStdDev;
}

uint8 At_Compliance() {
	Compliance_Reached = 0;
	
	float current = 0;
	float currentSD = 0;
	
	Measure_Current_Vss(&current, &currentSD, 3, 1);
	
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

// Get the reference voltage in volts
float Get_Ref() {
	float result = 4.080/255.0*VDAC_Ref_Data;
	if (VDAC_Ref_CR0 & (VDAC_Ref_RANGE_1V & VDAC_Ref_RANGE_MASK)) {
		result = 1.020/255.0*VDAC_Ref_Data;
	}
	return result;
}

// Get Vgs in volts
float Get_Vgs() {
	float result = 4.080/255.0*VDAC_Vgs_Data - Get_Ref();
	if (VDAC_Vgs_CR0 & (VDAC_Vgs_RANGE_1V & VDAC_Vgs_RANGE_MASK)) {
		result = 1.020/255.0*VDAC_Vgs_Data - Get_Ref();
	}
	return result;
}

// Get Vds in volts
float Get_Vds() {
	float result = 4.080/255.0*VDAC_Vds_Data - Get_Ref();
	if (VDAC_Vds_CR0 & (VDAC_Vds_RANGE_1V & VDAC_Vds_RANGE_MASK)) {
		result = 1.020/255.0*VDAC_Vds_Data - Get_Ref();
	}
	return result;
}


// ****************************
// Set the raw value of Vds
void Set_Vds_Raw(uint8 value) {
	Vds_Index_Goal_Relative = (int16)value - (int16)VDAC_Ref_Data;
	
	uint8 istart = VDAC_Vds_Data;
	int8 increment = 1;
	if (istart > value) increment = -1;
	
	// Ramp up to the Vds value
	for (uint8 i = istart; i != value; i += increment) {
		if (At_Compliance()) {
			for (uint8 j = i; j != istart; j -= increment) {
				VDAC_Vds_SetValue(j);
				
				if (!At_Compliance()) return;
			}
			if (At_Compliance()) {
				Handle_Compliance_Breach();
				return;
			}
		}
		
		VDAC_Vds_SetValue(i);
	}
	
	// Now that ramp is done, set the Vds value
	VDAC_Vds_SetValue(value);
}

// Set the raw value of Vgs
void Set_Vgs_Raw(uint8 value) {
	Vgs_Index_Goal_Relative = (int16)value - (int16)VDAC_Ref_Data;
	
	int8 increment = 1;
	uint8 istart = VDAC_Vgs_Data;
	if (value < istart) increment = -1;
	
	// Ramp up to the Vgs value
	for (uint8 i = istart; i != value; i += increment) {
		if (At_Compliance()) {
			for (uint8 j = i; j != istart; j -= increment) {
				VDAC_Vgs_SetValue(j);
				
				if (!At_Compliance()) return;
			}
			if (At_Compliance()) {
				Handle_Compliance_Breach();
				return;
			}
		}
		
		VDAC_Vgs_SetValue(i);
	}
	
	// Now that ramp is done, set the Vgs value
	VDAC_Vgs_SetValue(value);
}

// Set the raw value of the reference voltage
void Set_Ref_Raw(uint8 value) {
	if (value < 6) value = 6;
	
	int8 increment = 1;
	if (value < VDAC_Ref_Data) increment = -1;
	
	// Ramp up to the Reference value
	for (uint8 i = VDAC_Ref_Data; i != value; i += increment) {
		
		if (At_Compliance()) return;
		
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
// ****************************

// Set Vds and move the reference voltage if necessary
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
	
	uint16 Vds_minRefRaw = Vds_Index_Goal_Relative > 0? 0: -Vds_Index_Goal_Relative;
	uint16 Vds_maxRefRaw = Vds_Index_Goal_Relative > 0? 255 - Vds_Index_Goal_Relative: 255;
	uint16 Vgs_minRefRaw = Vgs_Index_Goal_Relative > 0? 0: -Vgs_Index_Goal_Relative;
	uint16 Vgs_maxRefRaw = Vgs_Index_Goal_Relative > 0? 255 - Vgs_Index_Goal_Relative: 255;
	
	uint16 newRefRaw = VDAC_Ref_Data;
	
	if (newRefRaw < Vgs_minRefRaw) newRefRaw = Vgs_minRefRaw;
	if (newRefRaw > Vgs_maxRefRaw) newRefRaw = Vgs_maxRefRaw;
	if (newRefRaw < Vds_minRefRaw) newRefRaw = Vds_minRefRaw;
	if (newRefRaw > Vds_maxRefRaw) newRefRaw = Vds_maxRefRaw;
	
	Set_Ref_Raw(newRefRaw);
}

// Set Vgs and move the reference voltage if necessary
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
	
	uint16 Vds_minRefRaw = Vds_Index_Goal_Relative > 0? 0: -Vds_Index_Goal_Relative;
	uint16 Vds_maxRefRaw = Vds_Index_Goal_Relative > 0? 255 - Vds_Index_Goal_Relative: 255;
	uint16 Vgs_minRefRaw = Vgs_Index_Goal_Relative > 0? 0: -Vgs_Index_Goal_Relative;
	uint16 Vgs_maxRefRaw = Vgs_Index_Goal_Relative > 0? 255 - Vgs_Index_Goal_Relative: 255;
	
	uint16 newRefRaw = VDAC_Ref_Data;
	
	if (newRefRaw < Vds_minRefRaw) newRefRaw = Vds_minRefRaw;
	if (newRefRaw > Vds_maxRefRaw) newRefRaw = Vds_maxRefRaw;
	if (newRefRaw < Vgs_minRefRaw) newRefRaw = Vgs_minRefRaw;
	if (newRefRaw > Vgs_maxRefRaw) newRefRaw = Vgs_maxRefRaw;
	
	Set_Ref_Raw(newRefRaw);
}
// ****************************

// Set Vgs by volts (good for readability, but requires passing floats)
void Set_Vgs(float voltage) {
	Set_Vgs_Rel((voltage/4.080)*255.0);
}

// Set Vds by volts (good for readability, but requires passing floats)
void Set_Vds(float voltage) {
	Set_Vds_Rel((voltage/4.080)*255.0);
}
// ****************************

// Set Vgs by mV (good for passing in integers instead of floats)
void Set_Vgs_mV(float mV) {
	Set_Vgs(mV/1000.0);
}

// Set Vds by mV (good for passing in integers instead of floats)
void Set_Vds_mV(float mV) {
	Set_Vds(mV/1000.0);
}
// ****************************

// Set Vgs and Vds to 0
void Zero_All_DACs() {
	Set_Vds_Raw(0);
	Set_Vgs_Raw(0);
	
	VDAC_Vds_SetValue(0);
	VDAC_Vgs_SetValue(0);
}

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

// Take a measurement of the system (Id - from delta-sigma ADC, Vgs, Vds, SAR1 ADC, SAR2 ADC)
void Measure(uint32 deltaSigmaSampleCount, uint32 SAR1_SampleCount, uint32 SAR2_SampleCount) {
	float IdsAverageAmps = 0;
	float IdsSDAmps = 0;
	
	Measure_Current_Vss(&IdsAverageAmps, &IdsSDAmps, deltaSigmaSampleCount, 0);
	
	int32 SAR1_Average = 0;
	int32 SAR1_SD = 0;
	int32 SAR2_Average = 0;
	int32 SAR2_SD = 0;
	
	SAR1_Measure_uV(&SAR1_Average, &SAR1_SD, SAR1_SampleCount);
	SAR2_Measure_uV(&SAR2_Average, &SAR2_SD, SAR2_SampleCount);
	
	float SAR1 = (1e-6/1e6) * SAR1_Average;
	float SAR2 = (1e-6) * SAR2_Average;
	
	sprintf(TransmitBuffer, "[%e,%f,%f,%e]\r\n", IdsAverageAmps, Get_Vgs(), Get_Vds(), SAR1);
	sendTransmitBuffer();
}

// Repeatedly take measurements of the system
void Measure_Multiple(uint32 n) {
	for (uint32 i = 0; i < n; i++) {
		Measure(100, 1, 10);
	}
}

// Take a basic gate sweep (hardcoded voltages, hardcoded number of data points)
void Measure_Sweep() {
	VDAC_Vds_SetValue(90);
	VDAC_Vgs_SetValue(254);
	
	for (uint16 Vgsi = 0; Vgsi < 256; Vgsi++) {
		VDAC_Vgs_SetValue(Vgsi);
		Measure(100, 1, 1);
	}
}

void Measure_Gate_Sweep(uint8 loop) {
	
	if (Vds_Index_Goal_Relative > 0) {
		Set_Ref_Raw(254 - Vds_Index_Goal_Relative);
	} else {
		Set_Ref_Raw(254);
	}
	
	int8 speed = 16;
	
	for (uint8 l = 0; l <= loop; l++) {
		int8 direction = 1*speed;
		uint8 istart = 0;
		if (Vds_Index_Goal_Relative < 0) {
			istart = -Vds_Index_Goal_Relative;
		}
		
		uint8 istop = 256-speed;
		
		if (l%2 == 1) {
			direction = -1*speed;
			istart = 256-speed;
			istop = 0;
		}
		
		for (uint8 i = istart; i != istop; i += direction) {
			for (uint8 j = 0; j <= 1; j++) {
				if (j == 1) Set_Ref_Raw(254 - i + 1);
				Set_Vgs_Raw(i);
				
				Measure(33, 1, 1);
				
				if (G_Stop) break;
				while (G_Pause);
			}
		}
	}
}

void Measure_Wide_Gate_Sweep(uint8 loop) {
	Set_Ref_Raw(254);
	
	int8 speed = 16;
	
	for (uint8 l = 0; l <= loop; l++) {
		int8 direction = 1*speed;
		uint8 istart = 0;
		uint8 istop = 256-speed;
		
		if (l%2 == 1) {
			direction = -1*speed;
			istart = 256-speed;
			istop = 0;
		}
		
		for (uint8 i = istart; i != istop; i += direction) {
			for (uint8 j = 0; j <= 1; j++) {
				
				if (j == 1) Set_Ref_Raw(254 - i + 1);
				Set_Vgs_Raw(i);
				
				Measure(33, 1, 1);
				
				if (G_Stop) break;
				while (G_Pause);
			}
		}
	}
}

void Measure_Gate_Sweep_New() {
	uint8 refTurn = 1;
	int8 refIncrement = 1;
	uint8 startRefIndex = 0;
	uint8 endRefIndex = 0;
	uint8 refIndex = startRefIndex;
	uint8 refArrived = 0;
	
	int8 gateIncrement = 1;
	uint8 startGateIndex = 0;
	uint8 endGateIndex = 0;
	uint8 gateIndex = startGateIndex;
	uint8 gateArrived = 0;
	
	
	while (!gateArrived || !refArrived) {
		gateArrived = abs(gateIndex-endGateIndex) < abs(gateIncrement);
		refArrived = abs(refIndex-endRefIndex) < abs(refIncrement);
		
		refTurn = !refTurn;
		
		if (refTurn && refArrived) continue;
		if (!refTurn && gateArrived) continue;
		
		Set_Ref_Raw(refIndex);
		Set_Vgs_Raw(gateIndex);
		
		Measure(100, 1, 1);
		
		if (refTurn) {
			refIndex += refIncrement;
		} else {
			gateIndex += gateIncrement;
		}
		
		if (G_Stop) break;
		while (G_Pause);
	}
}

void Scan(uint8 wide, uint8 loop) {
	for (uint8 device = 1; device <= CONTACT_COUNT/**//2; device++) {
		uint8 contact1 = device;
		uint8 contact2 = device + 1;
		uint8 intermediate1 = 1;
		uint8 intermediate2 = 2;
		
		Disconnect_All_Contacts_From_All_Intermediates();
		Zero_All_DACs();
		Connect_Intermediates();
		Connect_Contact_To_Intermediate(contact1, intermediate1);
		Connect_Contact_To_Intermediate(contact2, intermediate2);
		
		sprintf(TransmitBuffer, "\r\n%u\r\n", device);
		sendTransmitBuffer();
		
		if (wide) {
			Measure_Wide_Gate_Sweep(loop);
		} else {
			Measure_Gate_Sweep(loop);
		}
		
		if (G_Break || G_Stop) break;
		while (G_Pause);
	}
}

void Scan_Range(uint8 startDevice, uint8 stopDevice, uint8 wide, uint8 loop) {
	if (startDevice >= CONTACT_COUNT) return;
	if (stopDevice >= CONTACT_COUNT) return;
	
	for (uint8 device = startDevice; device <= stopDevice; device++) {
		uint8 contact1 = device;
		uint8 contact2 = device + 1;
		uint8 intermediate1 = 1;
		uint8 intermediate2 = 2;
		
		Disconnect_All_Contacts_From_All_Intermediates();
		Zero_All_DACs();
		Connect_Intermediates();
		Connect_Contact_To_Intermediate(contact1, intermediate1);
		Connect_Contact_To_Intermediate(contact2, intermediate2);
		
		sprintf(TransmitBuffer, "\r\n%u\r\n", device);
		sendTransmitBuffer();
		
		if (wide) {
			Measure_Wide_Gate_Sweep(loop);
		} else {
			Measure_Gate_Sweep(loop);
		}
		
		if (G_Break || G_Stop) break;
		while (G_Pause);
	}
}

CY_ISR (CommunicationHandlerISR) {
	if (USBUARTH_DataIsReady()) {
		// USBUARTH_Receive_Until(UART_Receive_Buffer, USBUART_BUFFER_SIZE, "\r");
		
		char temp_buffer[USBUART_BUFFER_SIZE];
		uint8 temp_count = USBUART_GetAll((uint8*) temp_buffer);
		
		for (uint8 i = 0; i < temp_count; i++) {
			char c = temp_buffer[i];
			
			USBUART_Receive_Buffer[USBUART_Rx_Position] = c;
			
			// sprintf(TransmitBuffer, "USB: %c\r\n", USBUART_Receive_Buffer[USBUART_Rx_Position]);
			// UART_1_PutString(TransmitBuffer);
			
			if (c == '\r' || c == '\n' || c == '!') {
				USBUART_Receive_Buffer[USBUART_Rx_Position] = 0;
				if (USBUART_Rx_Position) newData = 1;
				USBUART_Rx_Position = 0;
			} else {
				USBUART_Rx_Position++;
			}
			
			if (USBUART_Rx_Position >= USBUART_BUFFER_SIZE) USBUART_Rx_Position = 0;
		}
	} else
	if (UART_1_GetRxBufferSize()) {
		UART_Receive_Buffer[UART_Rx_Position] = UART_1_GetChar();
		
		if (UART_Receive_Buffer[UART_Rx_Position] == '\r' || UART_Receive_Buffer[UART_Rx_Position] == '\n' || UART_Receive_Buffer[UART_Rx_Position] == '!') {
			UART_Receive_Buffer[UART_Rx_Position] = 0;
			if (UART_Rx_Position) newData = 2;
			UART_Rx_Position = 0;
		} else {
			UART_Rx_Position++;
		}
		
		if (UART_Rx_Position >= USBUART_BUFFER_SIZE) UART_Rx_Position = 0;
	}
	
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

int main(void) {
	CyGlobalIntEnable;
	
	//Setup communication to the MUXes
	Setup_Selectors();
	
	//Start USB Interface
	USBUART_Start(0u, USBUART_5V_OPERATION);
	UART_1_Start();
	
	//Start All DACs, ADCs, and TIAs
	VDAC_Vds_Start();
	VDAC_Vgs_Start();
	VDAC_Ref_Start();
	ADC_DelSig_1_Start();
	TIA_1_Start();
	I2C_1_Start();
	ADC_SAR_1_Start();
	ADC_SAR_2_Start();
	
	//Start the op-amp buffers
	Opamp_1_Start();
	Opamp_2_Start();
	Opamp_3_Start();
	Opamp_4_Start();
	
	//Tell the ADC to begin sampling continuously
	ADC_DelSig_1_StartConvert();
	
	CyDelay(1000);
	
	// === All components now active ===
	
	UART_1_PutString("\r\n# Starting\r\n");
	
	// Connect S1, S2, S3, S4 signals to the MUXes
	Connect_Intermediates();
	
	// Prepare to receive commands from the host
	newData = 0;
	UART_Rx_Position = 0;
	USBUART_Rx_Position = 0;
	
	CommunicationTimer_Start();
	CommunicationInterrupt_StartEx(CommunicationHandlerISR);
	
	// === Ready to receive commands ===
	
	//Calibrate Delta-Sigma ADC and set initial current range
	Calibrate_ADC_Offset(300);
	TIA1_Set_Resistor(TIA1_Selected_Resistor);
	
	Current_Measurement_Sample_Count = 100;
	
	while (1) {
		G_Stop = 0;
		G_Break = 0;
		
		if (newData) {
			char* ReceiveBuffer = &USBUART_Receive_Buffer[0];
			if (newData == 2) ReceiveBuffer = &UART_Receive_Buffer[0];
			
			newData = 0;
			
			if (strstr(ReceiveBuffer, "measure ") == &ReceiveBuffer[0]) {
				Measure(100, 20, 20);
			} else 
			if (strstr(ReceiveBuffer, "measure-multiple ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint32 n = strtol(location, &location, 10);
				
				Measure_Multiple(n);
			} else 
			if (strstr(ReceiveBuffer, "measure-sweep ") == &ReceiveBuffer[0]) {
				Measure_Sweep();
			} else 
			if (strstr(ReceiveBuffer, "measure-gate-sweep ") == &ReceiveBuffer[0]) {
				Measure_Gate_Sweep(0);
			} else 
			if (strstr(ReceiveBuffer, "measure-gate-sweep-loop ") == &ReceiveBuffer[0]) {
				Measure_Gate_Sweep(1);
			} else 
			if (strstr(ReceiveBuffer, "calibrate-offset ") == &ReceiveBuffer[0]) {
				Calibrate_ADC_Offset(300);
			} else
			if (strstr(ReceiveBuffer, "set-vgs-raw ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 vgsi = strtol(location, &location, 10);
				
				Set_Vgs_Raw(vgsi);
			} else 
			if (strstr(ReceiveBuffer, "set-vds-raw ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 vdsi = strtol(location, &location, 10);
				
				Set_Vds_Raw(vdsi);
			} else 
			if (strstr(ReceiveBuffer, "set-vgs-rel ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				int8 vgsi = strtol(location, &location, 10);
				
				Set_Vgs_Rel(vgsi);
			} else 
			if (strstr(ReceiveBuffer, "set-vds-rel ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				int8 vdsi = strtol(location, &location, 10);
				
				Set_Vds_Rel(vdsi);
			} else 
			if (strstr(ReceiveBuffer, "set-vgs ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float vgs = 0;
				int8 success = sscanf(location, "%f", &vgs);
				
				Set_Vgs(vgs);
			} else 
			if (strstr(ReceiveBuffer, "set-vds ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				float vds = 0;
				int8 success = sscanf(location, "%f", &vds);
				
				Set_Vds(vds);
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
			if (strstr(ReceiveBuffer, "scan ") == &ReceiveBuffer[0]) {
				sprintf(TransmitBuffer, "\r\n# Scan Starting\r\n");
				sendTransmitBuffer();
				
				Scan(0, 0);
				
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
			if (strstr(ReceiveBuffer, "connect ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 contact = strtol(location, &location, 10);
				uint8 intermediate = strtol(location, &location, 10);
				Connect_Contact_To_Intermediate(contact, intermediate);
				
				sprintf(TransmitBuffer, "# Connected %u to %u\r\n", contact, intermediate);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-c ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 channel = strtol(location, &location, 10);
				uint8 intermediate = strtol(location, &location, 10);
				Connect_Channel_On_Intermediate(channel, intermediate);
				
				sprintf(TransmitBuffer, "# Connected channel %u to %u\r\n", channel, intermediate);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-c ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 channel = strtol(location, &location, 10);
				uint8 intermediate = strtol(location, &location, 10);
				Disconnect_Channel_On_Intermediate(channel, intermediate);
				
				sprintf(TransmitBuffer, "# Disconnected channel %u from %u\r\n", channel, intermediate);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 contact = strtol(location, &location, 10);
				uint8 intermediate = strtol(location, &location, 10);
				Disconnect_Contact_From_Intermediate(contact, intermediate);
				
				sprintf(TransmitBuffer, "# Disonnected %u from %u\r\n", contact, intermediate);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-all-from ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 intermediate = strtol(location, &location, 10);
				Disconnect_All_Contacts_From_Intermediate(intermediate);
				
				sprintf(TransmitBuffer, "# Disconnected all from  %u\r\n", intermediate);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-all-from-all ") == &ReceiveBuffer[0]) {
				Disconnect_All_Contacts_From_All_Intermediates();
				
				sprintf(TransmitBuffer, "# Disconnected all from all\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-from-all ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 contact = strtol(location, &location, 10);
				Disconnect_Contact_From_All_Intermediates(contact);
				
				sprintf(TransmitBuffer, "# Disconnected %u from all\r\n", contact);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-intermediate ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 intermediate = strtol(location, &location, 10);
				Connect_Intermediate(intermediate);
				
				sprintf(TransmitBuffer, "# Connected intermediate %u\r\n", intermediate);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-intermediate ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint8 intermediate = strtol(location, &location, 10);
				Disconnect_Intermediate(intermediate);
				
				sprintf(TransmitBuffer, "# Disconnected intermediate %u\r\n", intermediate);
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "connect-intermediates ") == &ReceiveBuffer[0]) {
				Connect_Intermediates();
				
				sprintf(TransmitBuffer, "# Connected intermediates\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "disconnect-intermediates ") == &ReceiveBuffer[0]) {
				Disconnect_Intermediates();
				
				sprintf(TransmitBuffer, "# Disconnected intermediates\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "Set_Current_Measurement_Sample_Count ") == &ReceiveBuffer[0]) {
				char* location = strstr(ReceiveBuffer, " ");
				uint32 sampleCount = strtol(location, &location, 10);
				
				Current_Measurement_Sample_Count = sampleCount;
				
				sprintf(TransmitBuffer, "# Set Current Measurement Sample Count\r\n");
				sendTransmitBuffer();
			} else 
			if (strstr(ReceiveBuffer, "enable-uart-sending ") == &ReceiveBuffer[0]) {
				sprintf(TransmitBuffer, "# Enabled UART Sending\r\n");
				sendTransmitBuffer();
				
				uartSendingEnabled = true;
			} else 
			if (strstr(ReceiveBuffer, "enable-usbu-sending ") == &ReceiveBuffer[0]) {
				sprintf(TransmitBuffer, "# Enabled USBU Sending\r\n");
				sendTransmitBuffer();
				
				usbuSendingEnabled = true;
			} else 
			if (strstr(ReceiveBuffer, "disable-uart-sending ") == &ReceiveBuffer[0]) {
				sprintf(TransmitBuffer, "# Disabled UART Sending\r\n");
				sendTransmitBuffer();
				
				uartSendingEnabled = false;
			} else 
			if (strstr(ReceiveBuffer, "disable-usbu-sending ") == &ReceiveBuffer[0]) {
				sprintf(TransmitBuffer, "# Disabled USBU Sending\r\n");
				sendTransmitBuffer();
				
				usbuSendingEnabled = false;
			} else {
				sprintf(TransmitBuffer, "! Unidentified command: |%s|\r\n", ReceiveBuffer);
				sendTransmitBuffer();
			}
		}
		
		//for (uint8 i = 0; i < SELECTOR_COUNT; i++) Update_Selector(i);
	}
}
