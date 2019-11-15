#include "project.h"
#include "stdio.h"

#if defined (__GNUC__)
	/* Add an explicit reference to the floating point printf library */
	/* to allow usage of the floating point conversion specifiers. */
	/* This is not linked in by default with the newlib-nano library. */
	asm (".global _printf_float");
#endif

// The buffer size is equal to the maximum packet size of the IN and OUT bulk endpoints.
#define USBUART_BUFFER_SIZE	(64u)
#define LINE_STR_LENGTH		(20u)


void USBUARTH_Maintain_Configuration() {
	unsigned long tries = 0;
	
	do {
		// Host can send double SET_INTERFACE request.
		if (USBUART_IsConfigurationChanged()) {
			// Initialize IN endpoints when device is configured.
			if (USBUART_GetConfiguration()) {
				// Enumeration is done, enable OUT endpoint to receive data from host.
				USBUART_CDC_Init();
			}
		}
		
		tries++;
	// Keep trying until USB CDC device is configured
	} while (!USBUART_GetConfiguration() && tries < 1);
}

uint8 USBUARTH_DataIsReady() {
	USBUARTH_Maintain_Configuration();
	
	return USBUART_GetConfiguration() && USBUART_DataIsReady();
}


uint8 USBUARTH_Send(char* buffer, uint16 count) {
	if (count > 0) {
		// Wait until component is ready to send data to host.
		for (uint32 tries = 0; tries < 10; tries++) {
			USBUARTH_Maintain_Configuration();
			if (USBUART_CDCIsReady()) break;
		}
        
		if (!USBUART_CDCIsReady()) return 0;
		
		// Send data to host
		USBUART_PutData((uint8*) buffer, count);
		
		/* If the last sent packet is exactly the maximum packet 
		*  size, it is followed by a zero-length packet to assure
		*  that the end of the segment is properly identified by 
		*  the terminal.
		*/
		if (USBUART_BUFFER_SIZE == count) {
			// Wait until component is ready to send data to PC.
			while (!USBUART_CDCIsReady());
            
			// Send zero-length packet to PC.
			USBUART_PutData(NULL, 0);
		}
	}
	
	return 1;
}

uint8 USBUARTH_Receive(char* buffer, uint16* count) {
	// Wait for input data from host
	while (!USBUART_DataIsReady()) USBUARTH_Maintain_Configuration();
	
	// Read received data and re-enable OUT endpoint.
	*count = USBUART_GetAll((uint8*) buffer);
	
	buffer[*count] = 0;
	
	return 1u;
}

char USBUARTH_GetChar() {
	while (!USBUART_DataIsReady()) USBUARTH_Maintain_Configuration();
	
	return USBUART_GetChar();
}

uint8 USBUARTH_Receive_Until(char* buffer, uint16 maxLength, const char* termination) {
	for (uint16 i = 0; i < maxLength; i++) {
		buffer[i] = 0;
	}
	
	for (uint16 i = 0; i < maxLength - 1; i++) {
		buffer[i] = USBUARTH_GetChar();
		buffer[i+1] = 0;
		
		if (strstr(buffer, termination)) {
			buffer[i + 1 - strlen(termination)] = 0;
			return 1;
		}
	}
	
	return 0;
}

