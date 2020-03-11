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
Text GLabel 5300 1050 0    50   Input ~ 0
VDD
Text GLabel 5300 1150 0    50   Input ~ 0
GND
Text GLabel 5300 1250 0    50   Input ~ 0
RX
Text GLabel 5300 1350 0    50   Input ~ 0
TX
$Comp
L Battery_Management:MCP73831-2-OT CGU1
U 1 1 5E4B705D
P 4900 2550
F 0 "CGU1" H 4900 3028 50  0000 C CNN
F 1 "MCP73831-2-OT" H 4900 2937 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-23-5" H 4950 2300 50  0001 L CIN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/20001984g.pdf" H 4750 2500 50  0001 C CNN
	1    4900 2550
	1    0    0    -1  
$EndComp
Text GLabel 3500 2250 0    50   Input ~ 0
VDD
$Comp
L Device:C C1
U 1 1 5E4B72EC
P 3700 2750
F 0 "C1" H 3815 2796 50  0000 L CNN
F 1 "10uF" H 3815 2705 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 3738 2600 50  0001 C CNN
F 3 "~" H 3700 2750 50  0001 C CNN
	1    3700 2750
	-1   0    0    1   
$EndComp
Text GLabel 3700 3050 3    50   Input ~ 0
GND
Wire Wire Line
	3700 3050 3700 2900
$Comp
L Connector_Generic:Conn_01x02 JSTPH1
U 1 1 5E4B78CE
P 6300 2450
F 0 "JSTPH1" H 6380 2442 50  0000 L CNN
F 1 "Conn_01x02" H 6380 2351 50  0000 L CNN
F 2 "Custom_Footprint_Library:JST Lipo Header (S2B-PH-SM4-TB)" H 6300 2450 50  0001 C CNN
F 3 "~" H 6300 2450 50  0001 C CNN
	1    6300 2450
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_01x04 J1
U 1 1 5E4B7A25
P 5500 1150
F 0 "J1" H 5580 1142 50  0000 L CNN
F 1 "Conn_01x04" H 5580 1051 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 5500 1150 50  0001 C CNN
F 3 "~" H 5500 1150 50  0001 C CNN
	1    5500 1150
	1    0    0    -1  
$EndComp
Text GLabel 5700 2250 1    50   Input ~ 0
VBAT
Wire Wire Line
	5700 2250 5700 2450
Connection ~ 5700 2450
Wire Wire Line
	5700 2450 5300 2450
$Comp
L Device:C C2
U 1 1 5E4B7CFC
P 5700 2750
F 0 "C2" H 5815 2796 50  0000 L CNN
F 1 "10uF" H 5815 2705 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 5738 2600 50  0001 C CNN
F 3 "~" H 5700 2750 50  0001 C CNN
	1    5700 2750
	1    0    0    -1  
$EndComp
Wire Wire Line
	5700 2900 5700 3050
Wire Wire Line
	5700 2450 5700 2600
Text GLabel 5700 3050 3    50   Input ~ 0
GND
Text GLabel 6100 3050 3    50   Input ~ 0
GND
Wire Wire Line
	5700 2450 6100 2450
Wire Wire Line
	6100 2550 6100 3050
Wire Wire Line
	4900 2850 4900 3050
Text GLabel 4900 3050 3    50   Input ~ 0
GND
Wire Wire Line
	3700 2250 3700 2600
Connection ~ 3700 2250
$Comp
L Device:LED D2
U 1 1 5E4B8767
P 4000 3600
F 0 "D2" V 4038 3483 50  0000 R CNN
F 1 "CHG" V 3947 3483 50  0000 R CNN
F 2 "LED_SMD:LED_0603_1608Metric" H 4000 3600 50  0001 C CNN
F 3 "~" H 4000 3600 50  0001 C CNN
	1    4000 3600
	0    -1   -1   0   
$EndComp
$Comp
L Device:R R2
U 1 1 5E4B896E
P 4000 4050
F 0 "R2" H 3930 4004 50  0000 R CNN
F 1 "1k" H 3930 4095 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 3930 4050 50  0001 C CNN
F 3 "~" H 4000 4050 50  0001 C CNN
	1    4000 4050
	-1   0    0    1   
$EndComp
Wire Wire Line
	4000 3750 4000 3900
Wire Wire Line
	4000 4200 4000 4350
Wire Wire Line
	5300 4350 5300 2650
$Comp
L Device:LED D3
U 1 1 5E4B9154
P 4650 3600
F 0 "D3" V 4595 3678 50  0000 L CNN
F 1 "DONE" V 4686 3678 50  0000 L CNN
F 2 "LED_SMD:LED_0603_1608Metric" H 4650 3600 50  0001 C CNN
F 3 "~" H 4650 3600 50  0001 C CNN
	1    4650 3600
	0    1    1    0   
$EndComp
$Comp
L Device:R R3
U 1 1 5E4B915B
P 4650 4050
F 0 "R3" H 4580 4004 50  0000 R CNN
F 1 "1k" H 4580 4095 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 4580 4050 50  0001 C CNN
F 3 "~" H 4650 4050 50  0001 C CNN
	1    4650 4050
	-1   0    0    1   
$EndComp
Wire Wire Line
	4650 3750 4650 3900
Wire Wire Line
	4650 4200 4650 4350
Text GLabel 4650 3250 1    50   Input ~ 0
GND
Wire Wire Line
	4650 4350 5300 4350
Wire Wire Line
	4650 3450 4650 3250
Wire Wire Line
	4000 3450 4000 2250
Connection ~ 4000 2250
Text GLabel 4300 3050 3    50   Input ~ 0
GND
Wire Wire Line
	3700 2250 4000 2250
$Comp
L Device:R RREF1
U 1 1 5E4BA9FB
P 4300 2850
F 0 "RREF1" H 4230 2804 50  0000 R CNN
F 1 "5k" H 4230 2895 50  0000 R CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 4230 2850 50  0001 C CNN
F 3 "~" H 4300 2850 50  0001 C CNN
	1    4300 2850
	-1   0    0    1   
$EndComp
Wire Wire Line
	4300 3000 4300 3050
Text Notes 4050 2600 0    50   ~ 0
10k: 100 mA\n5k: 200 mA\n2k: 500 mA\n1k: 1000 mA\n
Wire Wire Line
	3500 2250 3700 2250
Wire Wire Line
	4500 2650 4300 2650
Wire Wire Line
	4300 2650 4300 2700
Wire Wire Line
	4000 2250 4900 2250
Wire Wire Line
	4000 4350 4650 4350
Connection ~ 4650 4350
Text GLabel 4300 2650 0    50   Input ~ 0
PROG
Text GLabel 5300 2650 2    50   Input ~ 0
STAT
$Comp
L Device:D D1
U 1 1 5E4BA0EB
P 3700 2000
F 0 "D1" V 3746 1921 50  0000 R CNN
F 1 "D" V 3655 1921 50  0000 R CNN
F 2 "Diode_SMD:D_0603_1608Metric" H 3700 2000 50  0001 C CNN
F 3 "~" H 3700 2000 50  0001 C CNN
	1    3700 2000
	0    -1   -1   0   
$EndComp
Text GLabel 3700 1750 1    50   Input ~ 0
VBAT
Wire Wire Line
	3700 1750 3700 1850
Wire Wire Line
	3700 2150 3700 2250
$Comp
L Mechanical:MountingHole MH1
U 1 1 5E4C45A5
P 6650 1050
F 0 "MH1" H 6750 1096 50  0000 L CNN
F 1 "MountingHole" H 6750 1005 50  0000 L CNN
F 2 "Custom_Footprint_Library:MountingHole_2.2mm" H 6650 1050 50  0001 C CNN
F 3 "~" H 6650 1050 50  0001 C CNN
	1    6650 1050
	1    0    0    -1  
$EndComp
$Comp
L Mechanical:MountingHole MH2
U 1 1 5E4C463D
P 6650 1300
F 0 "MH2" H 6750 1346 50  0000 L CNN
F 1 "MountingHole" H 6750 1255 50  0000 L CNN
F 2 "Custom_Footprint_Library:MountingHole_2.2mm" H 6650 1300 50  0001 C CNN
F 3 "~" H 6650 1300 50  0001 C CNN
	1    6650 1300
	1    0    0    -1  
$EndComp
$EndSCHEMATC
