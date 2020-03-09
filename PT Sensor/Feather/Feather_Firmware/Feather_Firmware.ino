// === Constants ===
// "Serial" refers to the Feather's USB interface, while "Serial1" refers to the physical hardware RX/TX pins
#define usbSerial Serial
#define psocSerial Serial1

// Defines optional fixed delays (in milliseconds)
#define LOOP_DELAY_MS 0
#define CONTINUOUS_PRINT_DELAY_MS 500

// Defines the maximum length string to read/print over USB
#define USB_BUFFER_SIZE 64
#define USB_EOL_CHARACTER '!'

#define EXTERNAL_RESISTOR_OHMS 499



// === Variables ===
char TransmitBufferUSB[USB_BUFFER_SIZE];
char ReceiveBufferUSB[USB_BUFFER_SIZE];
byte ReceiveIndexUSB = 0;
boolean usbNewMessage = false;

boolean ModeConnectedToHost = false; 

float MeasurementImpedance = 0;



// === Setup & Loop ===

void setup() {
  Serial.begin(115200);
  psocSerial.begin(115200);
  Serial.println("Startup complete. System is ready.");
}

void loop() {
  // Receive and handle new data from PSoC
  int32_t PSoC_Measurement_uV = recvFromPSOC();
  updateMeasurement(PSoC_Measurement_uV);

  // Receive and handle messages from host CPU (if any)
  recvFromUSB();
  handleMessageFromUSB();

  // If not actively communicating with a host CPU, just continuously print measurements at a fixed interval
  if(!ModeConnectedToHost) {
    if(CONTINUOUS_PRINT_DELAY_MS) delay(CONTINUOUS_PRINT_DELAY_MS);
    printMeasurement();
  }

  if(LOOP_DELAY_MS) delay(LOOP_DELAY_MS);
}



// === Communication Functions ===

void recvFromUSB() {
  char rc;

  while (Serial.available() > 0 && usbNewMessage == false) {
    rc = Serial.read();
  
    if (rc != USB_EOL_CHARACTER) {
      ReceiveBufferUSB[ReceiveIndexUSB] = rc;
      ReceiveIndexUSB++;
      if (ReceiveIndexUSB >= USB_BUFFER_SIZE) {
        ReceiveIndexUSB = USB_BUFFER_SIZE - 1;
      }
    } else {
      ReceiveBufferUSB[ReceiveIndexUSB] = '\0';
      ReceiveIndexUSB = 0;
      usbNewMessage = true;
    }
  }
}

void handleMessageFromUSB() {
  if(usbNewMessage) {
    if(ReceiveBufferUSB == "CONN") {
      ModeConnectedToHost = true;
    } else if(ReceiveBufferUSB == "MEAS") {
      printMeasurement();
    }

    usbNewMessage = false;
  }
}

int32_t recvFromPSOC() {
  // Clear old data from PSoC
  while(psocSerial.available()) { psocSerial.read(); }

  // Return newest data from PSoC (this is a blocking call, timeout = 1 second)
  return psocSerial.parseInt();
}



// === Measurement Functions ===

// R1 is the sensor resistance
// R2 is the resistor resistance, 499 Ohms
// V2 is the value being measured
// R1 needs to be determined
// V1 + V2 = 0.3 V
// I1 = I2, V1/R1 = V2/R2
// R1 = R2 * V1/V2 = R2 * (0.3 - V2)/V2
void updateMeasurement(int32_t PSoC_Measurement_uV) {
  float VoltageRatio = ((float)(300000 - PSoC_Measurement_uV))/PSoC_Measurement_uV;
  MeasurementImpedance = EXTERNAL_RESISTOR_OHMS * VoltageRatio;
}

void printMeasurement() {
  sprintf(TransmitBufferUSB, "{'impedance':%f}", MeasurementImpedance);
  Serial.println(TransmitBufferUSB);
}
