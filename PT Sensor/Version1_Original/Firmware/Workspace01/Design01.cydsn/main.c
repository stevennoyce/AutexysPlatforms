#include<stdlib.h>
#include<stdio.h>
#include<stdbool.h>
#include "project.h"

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
                // sap elements
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
    
    for(;;)
    {
        char transmitBuffer[100];
        
//        int32 point = measurePoint();
//        sprintf(transmitBuffer, "%li\r\n", point);
//        uart_PutString(transmitBuffer);
        
        uint32 N = 11*5;
        int32 segment[N];
        
//        for (uint32 i = 0; i < N; i++) {
//            segment[i] = measurePoint();
//        }
//        
//        for (uint32 i = 0; i < N; i++) {
//            segment[i] = adcdelsig_CountsTo_uVolts(segment[i]);
//        }
        
//        for (uint32 i = 0; i < N; i++) {
//            sprintf(transmitBuffer, "%li\r\n", segment[i]);
//            uart_PutString(transmitBuffer);
//        }
        
        int magsN = 10;
        int32 mags[magsN];
        for (uint32 magi = 0; magi < N; magi++) {
            for (uint32 i = 0; i < N; i++) {
                segment[i] = measurePoint();
            }
            
            for (uint32 i = 0; i < N; i++) {
                segment[i] = adcdelsig_CountsTo_uVolts(segment[i]);
            }
            
            int32 max = 0;
            int32 min = 0;
            for (uint32 i = 0; i < N; i++) {
                if (segment[i] > max) max = segment[i];
                if (segment[i] < min) min = segment[i];
            }
            mags[magi] = max - min;
        }
        sortArray(mags, magsN);
        uint32 extremePointsToDrop = 2;
        uint32 mag = meanOfArray(mags + extremePointsToDrop, magsN - 2*extremePointsToDrop);
        
        sprintf(transmitBuffer, "%li\r\n", mag);
        uart_PutString(transmitBuffer);
        
        led_Write(!led_Read());
    }
}

/* [] END OF FILE */
