#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>
#include "project.h"

uint32 mag0;
uint32 mag1;
uint32 mag2;

int32 measurePoint() {
    while(!adcdelsig_IsEndConversion(adcdelsig_RETURN_STATUS));
    return adcdelsig_GetResult32();
    // return adcdelsig_CountsTo_uVolts(adcdelsig_GetResult32());
}

void sortArray(int32 array[], uint32 size) {
    int32 temp;
    // the following two loops sort the array in ascending order
    for (uint32 i = 0; i < size-1; i++) {
        for (uint32 j = i+1; j < size; j++) {
            if (array[j] < array[i]) {
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
    
    if (size%2 == 0) {
        // if size is even, return the mean of middle elements
        return ((array[size/2] + array[size/2 - 1])/2);
    } else {
        return array[size/2];
    }
}

int32 meanOfArray(int32 array[], uint32 size) {
    int64 sum = 0;
    for (uint32 i = 0; i < size; i++) {
        sum += array[i];
    }
    return (int32) (((int64)sum)/((int64)size));
}

int main(void)
{
	
    CyGlobalIntEnable; /* Enable global interrupts. */

    /* Place your initialization/startup code here (e.g. MyInst_Start()) */
    adcdelsig_Start();
    adcdelsig_StartConvert();
    buff_Start();
    dacref_Start();
    dacwave_Start();
    uart_Start();
	AMux_1_Start();
	AMux_2_Start();
	AMux_3_Start();
	
	uint32 N1 = 11*5;
	uint32 N0 = 11*5;
	uint32 N2 = 11*5;
    int32 segment1[N1];
	int32 segment0[N0];
	int32 segment2[N2];
	
	
    
    for(;;)
    {
        char transmitBuffer[100];
		uint32 extremePointsToDrop = 2;
        
        
			adcdelsig_StopConvert();		
			uint8 chan = 0;
			AMux_1_Select(chan);
			AMux_2_Select(chan);
			AMux_3_Select(chan);
			adcdelsig_StartConvert();
			
			int magsN0 = 10;
       		int32 mags0[magsN0];
       		for (uint32 magi0 = 0; magi0 < N0; magi0++) {
            	for (uint32 i = 0; i < N0; i++) {
                	segment0[i] = measurePoint();
            	}
            
            	for (uint32 i = 0; i < N0; i++) {
                	segment0[i] = adcdelsig_CountsTo_uVolts(segment0[i]);
            	}
            
            	int32 max0 = 0;
            	int32 min0 = 0;
            	for (uint32 i = 0; i < N0; i++) {
                	if (segment0[i] > max0) max0 = segment0[i];
                	if (segment0[i] < min0) min0 = segment0[i];
            	}
            	mags0[magi0] = max0 - min0;
        	}
        	sortArray(mags0, magsN0);
       		mag0 = meanOfArray(mags0 + extremePointsToDrop, magsN0 - 2*extremePointsToDrop);

			adcdelsig_StopConvert();
			chan = 1;
			AMux_1_Select(chan);
			AMux_2_Select(chan);
			AMux_3_Select(chan);
			adcdelsig_StartConvert();
        
        	int magsN1 = 10;
       		int32 mags1[magsN1];
       		for (uint32 magi1 = 0; magi1 < N1; magi1++) {
            	for (uint32 i = 0; i < N1; i++) {
                	segment1[i] = measurePoint();
            	}
            
            	for (uint32 i = 0; i < N1; i++) {
                	segment1[i] = adcdelsig_CountsTo_uVolts(segment1[i]);
            	}
            
            	int32 max1 = 0;
            	int32 min1 = 0;
            	for (uint32 i = 0; i < N1; i++) {
                	if (segment1[i] > max1) max1 = segment1[i];
                	if (segment1[i] < min1) min1 = segment1[i];
            	}
            	mags1[magi1] = max1 - min1;
        	}
        	sortArray(mags1, magsN1);
       		mag1 = meanOfArray(mags1 + extremePointsToDrop, magsN1 - 2*extremePointsToDrop);
			
			adcdelsig_StopConvert();
			chan = 2;
			AMux_1_Select(chan);
			AMux_2_Select(chan);
			AMux_3_Select(chan);
			adcdelsig_StartConvert();
        
        	int magsN2 = 10;
       		int32 mags2[magsN2];
       		for (uint32 magi2 = 0; magi2 < N2; magi2++) {
            	for (uint32 i = 0; i < N2; i++) {
                	segment2[i] = measurePoint();
            	}
            
            	for (uint32 i = 0; i < N2; i++) {
                	segment2[i] = adcdelsig_CountsTo_uVolts(segment2[i]);
            	}
            
            	int32 max2 = 0;
            	int32 min2 = 0;
            	for (uint32 i = 0; i < N2; i++) {
                	if (segment2[i] > max2) max2 = segment2[i];
                	if (segment2[i] < min2) min2 = segment2[i];
            	}
            	mags2[magi2] = max2 - min2;
        	}
        	sortArray(mags2, magsN2);
       		mag2 = meanOfArray(mags2 + extremePointsToDrop, magsN2 - 2*extremePointsToDrop);
		
		
		sprintf(transmitBuffer, "%li,%li,%li\r\n", mag0, mag1, mag2);
        uart_PutString(transmitBuffer);
        
        led_Write(!led_Read());
		
    }
}

/* [] END OF FILE */
