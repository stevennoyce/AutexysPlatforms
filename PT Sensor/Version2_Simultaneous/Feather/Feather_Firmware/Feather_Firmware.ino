// === Constants ===
// "Serial" refers to the Feather's USB interface, while "Serial1" refers to the physical hardware RX/TX pins
#define usbSerial Serial
#define psocSerial Serial1

// Defines optional fixed delays (in milliseconds)
#define LOOP_DELAY_MS 0
#define CONTINUOUS_PRINT_DELAY_MS 250

// Defines the maximum length string to read/print over USB
#define USB_BUFFER_SIZE 64
#define USB_EOL_CHARACTER '!'

#define EXTERNAL_RESISTOR_OHMS 499

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#define STMPE_CS 6
#define TFT_CS 9
#define TFT_DC 10
#define SD_CS 5




// === Variables ===
char TransmitBufferUSB[USB_BUFFER_SIZE];
char ReceiveBufferUSB[USB_BUFFER_SIZE];
byte ReceiveIndexUSB = 0;
boolean usbNewMessage = false;

boolean ModeConnectedToHost = false; 

float MeasurementImpedance = 0;

int32_t PSoC_Measurement_uV0;
int32_t PSoC_Measurement_uV1;
int32_t PSoC_Measurement_uV2;
char recChar[1000];
float MeasurementImpedance0 = 0;
float MeasurementImpedance1 = 0;
float MeasurementImpedance2 = 0;


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);




// === Setup & Loop ===

void setup() {
  asm(".global _printf_float");
  
  Serial.begin(115200);
  psocSerial.begin(115200);
  Serial.println("Startup complete. System is ready.");

  tft.begin();

  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
}


void loop() {
  // Receive and handle new data from PSoC
    for (int i=0; i<1000; i++){
    recChar[i] = recvFromPSOC();
    }

    Split();
    updateMeasurement();
    testText();
   

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

unsigned long testText(){
   tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(0, 0);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(5);
    tft.println(MeasurementImpedance0);
    tft.println(MeasurementImpedance1);
    tft.println(MeasurementImpedance2);
}


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
    if(strcmp(ReceiveBufferUSB, "CONN") == 0) {
      ModeConnectedToHost = true;
      Serial.println("# Connected to Host CPU.");
    } else if(strcmp(ReceiveBufferUSB, "MEAS") == 0) {
      printMeasurement();
    } else if(strcmp(ReceiveBufferUSB, "RESET") == 0) {
      ModeConnectedToHost = false;
      Serial.println("# Disconnected from Host CPU.");
    }

    usbNewMessage = false;
  }
}

int32_t recvFromPSOC() {
//   Clear old data from PSoC
  while(psocSerial.available()) {return psocSerial.read(); }
}

void Split() {
  // Split up the values
   char * strtokIndx; // this is used by strtok() as an index
  
  strtokIndx = strtok(recChar,","); //gets the first number
  PSoC_Measurement_uV0 = atoi(strtokIndx);     // convert this part to an integer
  
  strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
  PSoC_Measurement_uV1 = atoi(strtokIndx);     // convert this part to an integer

  strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
  PSoC_Measurement_uV2 = atoi(strtokIndx);     // convert this part to an integer
}



// === Measurement Functions ===

// R1 is the sensor resistance
// R2 is the resistor resistance, 499 Ohms
// V2 is the value being measured
// R1 needs to be determined
// V1 + V2 = 0.3 V
// I1 = I2, V1/R1 = V2/R2
// R1 = R2 * V1/V2 = R2 * (0.3 - V2)/V2
void updateMeasurement() {
  float VoltageRatio0 = ((float)(300000 - PSoC_Measurement_uV0))/PSoC_Measurement_uV0;
  MeasurementImpedance0 = EXTERNAL_RESISTOR_OHMS * VoltageRatio0;
  float VoltageRatio1 = ((float)(300000 - PSoC_Measurement_uV1))/PSoC_Measurement_uV1;
  MeasurementImpedance1 = EXTERNAL_RESISTOR_OHMS * VoltageRatio1;
  float VoltageRatio2 = ((float)(300000 - PSoC_Measurement_uV2))/PSoC_Measurement_uV2;
  MeasurementImpedance2 = EXTERNAL_RESISTOR_OHMS * VoltageRatio2;
}

void printMeasurement() {
  sprintf(TransmitBufferUSB, "{\"impedance\":[%f,%f,%f]}", MeasurementImpedance0, MeasurementImpedance1, MeasurementImpedance2);
  Serial.println(TransmitBufferUSB);
}
