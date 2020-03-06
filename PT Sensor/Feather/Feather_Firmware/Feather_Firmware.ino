// "Serial" refers to the Feather's USB interface, while "Serial1" refers to the physical hardware RX/TX pins
#define usbSerial Serial
#define psocSerial Serial1

void setup() {
  Serial.begin(115200);
  psocSerial.begin(115200);
}

void loop() {
  delay(500);
  
  int32_t mag_uv = psocSerial.parseInt();
 
  // R1 is the sensor resistance
  // R2 is the resistor resistance, 499 Ohms
  // V2 is the value being measured
  // R1 needs to be determined
  // V1 + V2 = 0.3 V
  // I1 = I2, V1/R1 = V2/R2
  // R1 = R2 * V1/V2 = R2 * (0.3 - V2)/V2
 
  float V2 = (float)mag_uv/1.0e6;
  float R2 = 499;
  float R1 = R2 * (0.3 - V2)/V2;
  Serial.println(R1);
}
