EESchema Schematic File Version 4
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text GLabel 1200 800  0    50   Input ~ 0
VDD
Text GLabel 1200 900  0    50   Input ~ 0
GND
Text GLabel 1200 1100 0    50   Input ~ 0
RX
Text GLabel 1200 1000 0    50   Input ~ 0
TX
$Comp
L Battery_Management:MCP73831-2-OT CGU1
U 1 1 5E4B705D
P 2550 2950
F 0 "CGU1" H 2550 3428 50  0000 C CNN
F 1 "MCP73831-2-OT" H 2550 3337 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-23-5" H 2600 2700 50  0001 L CIN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/20001984g.pdf" H 2400 2900 50  0001 C CNN
	1    2550 2950
	1    0    0    -1  
$EndComp
Text GLabel 1150 2650 0    50   Input ~ 0
VDD
$Comp
L Device:C C1
U 1 1 5E4B72EC
P 1350 3150
F 0 "C1" H 1465 3196 50  0000 L CNN
F 1 "10uF" H 1465 3105 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 1388 3000 50  0001 C CNN
F 3 "~" H 1350 3150 50  0001 C CNN
	1    1350 3150
	-1   0    0    1   
$EndComp
Text GLabel 1350 3450 3    50   Input ~ 0
GND
Wire Wire Line
	1350 3450 1350 3300
$Comp
L Connector_Generic:Conn_01x02 JSTPH1
U 1 1 5E4B78CE
P 3950 2850
F 0 "JSTPH1" H 4030 2842 50  0000 L CNN
F 1 "Conn_01x02" H 4030 2751 50  0000 L CNN
F 2 "Custom_Footprint_Library:JST Lipo Header (S2B-PH-SM4-TB)" H 3950 2850 50  0001 C CNN
F 3 "~" H 3950 2850 50  0001 C CNN
	1    3950 2850
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x04 J1
U 1 1 5E4B7A25
P 1400 900
F 0 "J1" H 1480 892 50  0000 L CNN
F 1 "Conn_01x04" H 1480 801 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 1400 900 50  0001 C CNN
F 3 "~" H 1400 900 50  0001 C CNN
	1    1400 900 
	1    0    0    -1  
$EndComp
Text GLabel 3350 2650 1    50   Input ~ 0
VBAT
Wire Wire Line
	3350 2650 3350 2850
Connection ~ 3350 2850
Wire Wire Line
	3350 2850 2950 2850
$Comp
L Device:C C2
U 1 1 5E4B7CFC
P 3350 3150
F 0 "C2" H 3465 3196 50  0000 L CNN
F 1 "10uF" H 3465 3105 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 3388 3000 50  0001 C CNN
F 3 "~" H 3350 3150 50  0001 C CNN
	1    3350 3150
	1    0    0    -1  
$EndComp
Wire Wire Line
	3350 3300 3350 3450
Wire Wire Line
	3350 2850 3350 3000
Text GLabel 3350 3450 3    50   Input ~ 0
GND
Text GLabel 3750 3450 3    50   Input ~ 0
GND
Wire Wire Line
	3350 2850 3750 2850
Wire Wire Line
	3750 2950 3750 3450
Wire Wire Line
	2550 3250 2550 3450
Text GLabel 2550 3450 3    50   Input ~ 0
GND
Wire Wire Line
	1350 2650 1350 3000
Connection ~ 1350 2650
$Comp
L Device:LED D2
U 1 1 5E4B8767
P 1650 4000
F 0 "D2" V 1688 3883 50  0000 R CNN
F 1 "CHG" V 1597 3883 50  0000 R CNN
F 2 "LED_SMD:LED_0603_1608Metric" H 1650 4000 50  0001 C CNN
F 3 "~" H 1650 4000 50  0001 C CNN
	1    1650 4000
	0    -1   -1   0   
$EndComp
$Comp
L Device:R R2
U 1 1 5E4B896E
P 1650 4450
F 0 "R2" H 1580 4404 50  0000 R CNN
F 1 "1k" H 1580 4495 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 1580 4450 50  0001 C CNN
F 3 "~" H 1650 4450 50  0001 C CNN
	1    1650 4450
	-1   0    0    1   
$EndComp
Wire Wire Line
	1650 4150 1650 4300
Wire Wire Line
	1650 4600 1650 4750
Wire Wire Line
	2950 4750 2950 3050
$Comp
L Device:LED D3
U 1 1 5E4B9154
P 2300 4000
F 0 "D3" V 2245 4078 50  0000 L CNN
F 1 "DONE" V 2336 4078 50  0000 L CNN
F 2 "LED_SMD:LED_0603_1608Metric" H 2300 4000 50  0001 C CNN
F 3 "~" H 2300 4000 50  0001 C CNN
	1    2300 4000
	0    1    1    0   
$EndComp
$Comp
L Device:R R3
U 1 1 5E4B915B
P 2300 4450
F 0 "R3" H 2230 4404 50  0000 R CNN
F 1 "1k" H 2230 4495 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 2230 4450 50  0001 C CNN
F 3 "~" H 2300 4450 50  0001 C CNN
	1    2300 4450
	-1   0    0    1   
$EndComp
Wire Wire Line
	2300 4150 2300 4300
Wire Wire Line
	2300 4600 2300 4750
Text GLabel 2300 3650 1    50   Input ~ 0
GND
Wire Wire Line
	2300 4750 2950 4750
Wire Wire Line
	2300 3850 2300 3650
Wire Wire Line
	1650 3850 1650 2650
Connection ~ 1650 2650
Text GLabel 1950 3450 3    50   Input ~ 0
GND
Wire Wire Line
	1350 2650 1650 2650
$Comp
L Device:R RREF1
U 1 1 5E4BA9FB
P 1950 3250
F 0 "RREF1" H 1880 3204 50  0000 R CNN
F 1 "5k" H 1880 3295 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 1880 3250 50  0001 C CNN
F 3 "~" H 1950 3250 50  0001 C CNN
	1    1950 3250
	-1   0    0    1   
$EndComp
Wire Wire Line
	1950 3400 1950 3450
Text Notes 1700 3000 0    50   ~ 0
10k: 100 mA\n5k: 200 mA\n2k: 500 mA\n1k: 1000 mA\n
Wire Wire Line
	1150 2650 1350 2650
Wire Wire Line
	2150 3050 1950 3050
Wire Wire Line
	1950 3050 1950 3100
Wire Wire Line
	1650 2650 2550 2650
Wire Wire Line
	1650 4750 2300 4750
Connection ~ 2300 4750
Text GLabel 1950 3050 0    50   Input ~ 0
PROG
Text GLabel 2950 3050 2    50   Input ~ 0
STAT
$Comp
L Device:D D1
U 1 1 5E4BA0EB
P 1350 2400
F 0 "D1" V 1396 2321 50  0000 R CNN
F 1 "D" V 1305 2321 50  0000 R CNN
F 2 "Diode_SMD:D_SOD-123F" H 1350 2400 50  0001 C CNN
F 3 "~" H 1350 2400 50  0001 C CNN
	1    1350 2400
	0    -1   -1   0   
$EndComp
Text GLabel 1350 2150 1    50   Input ~ 0
VBAT
Wire Wire Line
	1350 2150 1350 2250
Wire Wire Line
	1350 2550 1350 2650
Wire Notes Line width 20 style solid
	4800 5100 650  5100
Wire Notes Line width 20 style solid
	650  1600 4800 1600
Text Notes 1800 1900 0    79   ~ 0
Battery Charging Circuit\n
$Comp
L Regulator_Linear:AP2127K-3.3 REG1
U 1 1 5E69E213
P 2450 6250
F 0 "REG1" H 2450 6592 50  0000 C CNN
F 1 "AP2112K-3.3" H 2450 6501 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-23-5" H 2450 6575 50  0001 C CNN
F 3 "https://www.diodes.com/assets/Datasheets/AP2127.pdf" H 2450 6350 50  0001 C CNN
	1    2450 6250
	1    0    0    -1  
$EndComp
Text GLabel 1200 6150 0    50   Input ~ 0
VDD
Wire Wire Line
	2150 6250 2050 6250
Wire Wire Line
	2050 6250 2050 6150
Connection ~ 2050 6150
Wire Wire Line
	2050 6150 2150 6150
Text GLabel 2450 6700 3    50   Input ~ 0
GND
Wire Wire Line
	2450 6550 2450 6700
Wire Wire Line
	2750 6150 2950 6150
Text GLabel 3700 6150 2    50   Input ~ 0
V3_3
$Comp
L Device:C C4
U 1 1 5E6A058C
P 1850 6450
F 0 "C4" H 1965 6496 50  0000 L CNN
F 1 "10uF" H 1965 6405 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 1888 6300 50  0001 C CNN
F 3 "~" H 1850 6450 50  0001 C CNN
	1    1850 6450
	1    0    0    -1  
$EndComp
$Comp
L Device:C C3
U 1 1 5E6A0668
P 1400 6450
F 0 "C3" H 1515 6496 50  0000 L CNN
F 1 "1uF" H 1515 6405 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 1438 6300 50  0001 C CNN
F 3 "~" H 1400 6450 50  0001 C CNN
	1    1400 6450
	1    0    0    -1  
$EndComp
Wire Wire Line
	1200 6150 1400 6150
Wire Wire Line
	1400 6150 1400 6300
Wire Wire Line
	1400 6150 1850 6150
Connection ~ 1400 6150
Wire Wire Line
	1850 6300 1850 6150
Connection ~ 1850 6150
Wire Wire Line
	1850 6150 2050 6150
Text GLabel 1850 6700 3    50   Input ~ 0
GND
Text GLabel 1400 6700 3    50   Input ~ 0
GND
Wire Wire Line
	1400 6600 1400 6700
Wire Wire Line
	1850 6600 1850 6700
$Comp
L Device:C C5
U 1 1 5E6A255F
P 2950 6450
F 0 "C5" H 3065 6496 50  0000 L CNN
F 1 "10uF" H 3065 6405 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 2988 6300 50  0001 C CNN
F 3 "~" H 2950 6450 50  0001 C CNN
	1    2950 6450
	1    0    0    -1  
$EndComp
$Comp
L Device:C C6
U 1 1 5E6A25B1
P 3400 6450
F 0 "C6" H 3515 6496 50  0000 L CNN
F 1 "1uF" H 3515 6405 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 3438 6300 50  0001 C CNN
F 3 "~" H 3400 6450 50  0001 C CNN
	1    3400 6450
	1    0    0    -1  
$EndComp
Wire Wire Line
	2950 6150 2950 6300
Connection ~ 2950 6150
Wire Wire Line
	3400 6150 3400 6300
Wire Wire Line
	3400 6150 3700 6150
Connection ~ 3400 6150
Wire Wire Line
	2950 6150 3400 6150
Text GLabel 2950 6700 3    50   Input ~ 0
GND
Text GLabel 3400 6700 3    50   Input ~ 0
GND
Wire Wire Line
	2950 6600 2950 6700
Wire Wire Line
	3400 6600 3400 6700
Wire Notes Line width 20 style solid
	4800 600  4800 7650
Text Notes 1750 5550 0    79   ~ 0
3.3 V Linear Regulator
$Comp
L Device:LED D4
U 1 1 5E69C45C
P 7550 3500
F 0 "D4" V 7588 3383 50  0000 R CNN
F 1 "LED1_STATUS" V 7497 3383 50  0000 R CNN
F 2 "LED_SMD:LED_0603_1608Metric" H 7550 3500 50  0001 C CNN
F 3 "~" H 7550 3500 50  0001 C CNN
	1    7550 3500
	0    -1   -1   0   
$EndComp
$Comp
L Device:R R4
U 1 1 5E69CA38
P 7550 4000
F 0 "R4" H 7480 3954 50  0000 R CNN
F 1 "1k" H 7480 4045 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 7480 4000 50  0001 C CNN
F 3 "~" H 7550 4000 50  0001 C CNN
	1    7550 4000
	-1   0    0    1   
$EndComp
Wire Wire Line
	7550 3650 7550 3850
Text GLabel 7550 4350 3    50   Input ~ 0
GND
Wire Wire Line
	7550 4150 7550 4350
$Comp
L Device:R R6
U 1 1 5E6A6224
P 7550 2450
F 0 "R6" H 7480 2404 50  0000 R CNN
F 1 "10k" H 7480 2495 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 7480 2450 50  0001 C CNN
F 3 "~" H 7550 2450 50  0001 C CNN
	1    7550 2450
	-1   0    0    1   
$EndComp
$Comp
L Jumper:SolderJumper_2_Open JP1
U 1 1 5E6A6FEF
P 8000 2800
F 0 "JP1" H 8000 3005 50  0000 C CNN
F 1 "SolderJumper_2_Open" H 8000 2914 50  0000 C CNN
F 2 "Jumper:SolderJumper-2_P1.3mm_Open_RoundedPad1.0x1.5mm" H 8000 2800 50  0001 C CNN
F 3 "~" H 8000 2800 50  0001 C CNN
	1    8000 2800
	1    0    0    -1  
$EndComp
Wire Wire Line
	7550 2600 7550 2800
Text GLabel 7550 2100 1    50   Input ~ 0
GND
Wire Wire Line
	7550 2100 7550 2300
Text GLabel 8250 2800 2    50   Input ~ 0
V3_3
Wire Wire Line
	8150 2800 8250 2800
Wire Wire Line
	7550 3350 7550 3200
$Comp
L Custom_Schematic_Library:EGBT-045MS BTU1
U 1 1 5E69D27D
P 7050 3000
F 0 "BTU1" H 7050 3465 50  0000 C CNN
F 1 "EGBT-045MS" H 7050 3374 50  0000 C CNN
F 2 "Custom_Footprint_Library:EGBT-045MS" H 7050 2800 50  0001 C CNN
F 3 "" H 7050 2800 50  0001 C CNN
	1    7050 3000
	1    0    0    -1  
$EndComp
Text GLabel 6550 3100 0    50   Input ~ 0
V3_3
Wire Wire Line
	6550 3100 6650 3100
Text GLabel 6550 3200 0    50   Input ~ 0
GND
Wire Wire Line
	6550 3200 6650 3200
Text GLabel 6550 2800 0    50   Input ~ 0
TX
Wire Wire Line
	6550 2800 6650 2800
Text GLabel 6550 2900 0    50   Input ~ 0
RX
Wire Wire Line
	6550 2900 6650 2900
Wire Wire Line
	7450 3200 7550 3200
Wire Wire Line
	7450 3100 8300 3100
Text GLabel 7550 3200 2    50   Input ~ 0
LED1
Text GLabel 8300 3100 2    50   Input ~ 0
LED2
Wire Wire Line
	7850 2800 7550 2800
Wire Wire Line
	7450 2800 7550 2800
Connection ~ 7550 2800
Text GLabel 7000 3650 3    50   Input ~ 0
GND
Text GLabel 7100 3650 3    50   Input ~ 0
GND
Wire Wire Line
	7100 3550 7100 3650
Wire Wire Line
	7000 3550 7000 3650
$EndSCHEMATC
