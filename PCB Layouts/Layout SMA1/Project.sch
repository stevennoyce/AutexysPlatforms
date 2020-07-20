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
$Comp
L Connector_Generic:Conn_02x06_Odd_Even J1
U 1 1 5F0DCE5B
P 1400 1350
F 0 "J1" H 1450 1767 50  0000 C CNN
F 1 "Conn_02x06_Odd_Even" H 1450 1676 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_2x06_P2.54mm_Vertical" H 1400 1350 50  0001 C CNN
F 3 "~" H 1400 1350 50  0001 C CNN
	1    1400 1350
	1    0    0    -1  
$EndComp
Text GLabel 1100 1150 0    50   Input ~ 0
DD1
Text GLabel 1100 1250 0    50   Input ~ 0
DD2
Text GLabel 1100 1350 0    50   Input ~ 0
GND
Text GLabel 1100 1450 0    50   Input ~ 0
BackGate
Text GLabel 1100 1550 0    50   Input ~ 0
LocalGate
Text GLabel 1100 1650 0    50   Input ~ 0
SolutionGate
Wire Wire Line
	1100 1150 1200 1150
Wire Wire Line
	1100 1250 1200 1250
Wire Wire Line
	1100 1350 1200 1350
Wire Wire Line
	1100 1450 1200 1450
Wire Wire Line
	1100 1550 1200 1550
Wire Wire Line
	1100 1650 1200 1650
Text GLabel 1800 1150 2    50   Input ~ 0
SS1
Text GLabel 1800 1250 2    50   Input ~ 0
SS2
Text GLabel 1800 1550 2    50   Input ~ 0
Gate
Wire Wire Line
	1800 1550 1750 1550
Wire Wire Line
	1750 1550 1750 1450
Wire Wire Line
	1750 1450 1700 1450
Wire Wire Line
	1700 1550 1750 1550
Connection ~ 1750 1550
Wire Wire Line
	1700 1650 1750 1650
Wire Wire Line
	1750 1650 1750 1550
Wire Wire Line
	1800 1150 1700 1150
Wire Wire Line
	1800 1250 1700 1250
$Comp
L Connector_Generic:Conn_01x02 SIG1
U 1 1 5F0DD225
P 2550 1650
F 0 "SIG1" V 2423 1730 50  0000 L CNN
F 1 "Conn_01x02" V 2514 1730 50  0000 L CNN
F 2 "Custom_Footprint_Library:SMA_Vertical_ThroughHole" H 2550 1650 50  0001 C CNN
F 3 "~" H 2550 1650 50  0001 C CNN
	1    2550 1650
	0    1    1    0   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 SIG2
U 1 1 5F0DD68F
P 3400 1650
F 0 "SIG2" V 3273 1730 50  0000 L CNN
F 1 "Conn_01x02" V 3364 1730 50  0000 L CNN
F 2 "Custom_Footprint_Library:SMA_Vertical_ThroughHole" H 3400 1650 50  0001 C CNN
F 3 "~" H 3400 1650 50  0001 C CNN
	1    3400 1650
	0    1    1    0   
$EndComp
$Comp
L Connector_Generic:Conn_01x02 SIG3
U 1 1 5F0DD798
P 4200 1650
F 0 "SIG3" V 4073 1730 50  0000 L CNN
F 1 "Conn_01x02" V 4164 1730 50  0000 L CNN
F 2 "Custom_Footprint_Library:SMA_Vertical_ThroughHole" H 4200 1650 50  0001 C CNN
F 3 "~" H 4200 1650 50  0001 C CNN
	1    4200 1650
	0    1    1    0   
$EndComp
Text GLabel 2450 1350 1    50   Input ~ 0
GND
Wire Wire Line
	2450 1350 2450 1450
Text GLabel 3300 1350 1    50   Input ~ 0
GND
Wire Wire Line
	3300 1350 3300 1450
Text GLabel 4100 1350 1    50   Input ~ 0
GND
Wire Wire Line
	4100 1350 4100 1450
Text GLabel 2550 1150 1    50   Input ~ 0
DD1
Wire Wire Line
	2550 1150 2550 1450
Text GLabel 4200 1150 1    50   Input ~ 0
SS1
Wire Wire Line
	4200 1150 4200 1450
Text GLabel 3400 1150 1    50   Input ~ 0
Gate
Wire Wire Line
	3400 1150 3400 1450
$EndSCHEMATC
