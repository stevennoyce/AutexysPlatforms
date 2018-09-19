#include <stdlib.h>
#include <stdio.h>

#include "DHT.h"

#define DHTTYPE DHT11   //DHT11 Sensor


/**
 * 
 */

/*** CONSTANTS ***********************/ 
#define INDICATOR_LED 13

#define DHT_DATA_PIN 7

#define INPUT_BUFFER_SIZE 32
/*************************************/ 



/*** GLOBAL VARIABLES ****************/ 
static boolean indicator_toggle = 0;

static float humidity = 0.0;
static float temperature_celsius = 0.0;
static float temperature_fahrenheit = 0.0;

static char input_buffer[INPUT_BUFFER_SIZE];
static boolean new_input = false;

static DHT dht(DHT_DATA_PIN, DHTTYPE);
/*************************************/ 



/*** MAIN ****************************/ 
void setup()
{
  // configure an indicator LED to blink on sensor updates
  pinMode(INDICATOR_LED, OUTPUT);

  // initialize DHT sensor
  dht.begin();
  read_DHT_sensor();

  // begin serial communication
  Serial.begin(9600);
  Serial.println("Arduino booted up.");

  // allow timers to interrupt for data collection
  init_interrupts();
}
 
void loop()
{
  delay(100);       

  // Continuously watch for new commands and respond if necessary
  serial_read_input();
  serial_respond();  
}
/*************************************/ 



/*** INTERRUPTS **********************/ 
void init_interrupts()
{
  cli(); //stop interrupts

  //*** Set timer1 interrupt at 0.25Hz ***
  TCCR1A = 0; //set entire TCCR1A register to 0
  TCCR1B = 0; //same for TCCR1B
  TCNT1  = 0; //initialize counter value to 0
  // set compare match register for 0.25hz increments
  OCR1A = 62496;// = (16*10^6) / (0.25*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  
  sei(); //start interrupts
}

//timer1 interrupt handler
ISR(TIMER1_COMPA_vect){
  // Get the latest measurements from the DHT sensor
  read_DHT_sensor();

  // Toggle an indicator LED to show data collection occurred
  if (indicator_toggle){
    digitalWrite(INDICATOR_LED,HIGH);
    indicator_toggle = 0;
  } else {
    digitalWrite(INDICATOR_LED,LOW);
    indicator_toggle = 1;
  }
}
/*************************************/ 



/*** DHT22/DHT11 SENSOR **************/ 
void read_DHT_sensor() 
{
  float humid = dht.readHumidity();
  float celsius = dht.readTemperature();
  float fahrenheit = dht.readTemperature(true); // Read temperature as Fahrenheit (isFahrenheit = true)

  if (isnan(humid) || isnan(celsius) || isnan(fahrenheit)) {
    //Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  set_temperature_readings(humid, celsius, fahrenheit);
}

void set_temperature_readings(float humid, float temp_celsius, float temp_fahrenheit)
{
  humidity = humid;
  temperature_celsius = temp_celsius;
  temperature_fahrenheit = temp_fahrenheit;
}
/*************************************/ 



/*** SERIAL **************************/ 
//Check the serial interface for new incoming data and read it to global "input_buffer".
void serial_read_input() {
  static byte index = 0;
  char endMarker = '!';
  char next_char;

  if(Serial.available() > 0 && new_input == false) {
    for(int i = 0; i < INPUT_BUFFER_SIZE; i++){
      input_buffer[i] = '\0';
    }
    
    while (Serial.available() > 0 && new_input == false) {
      next_char = Serial.read();

      if(next_char == endMarker) {
        break;
      }

      input_buffer[index] = next_char;
      index++;
      if (index >= INPUT_BUFFER_SIZE) {
        index = INPUT_BUFFER_SIZE - 1;
      }
    }
      
    input_buffer[index] = '\0';
    index = 0;
    new_input = true;
    while((Serial.available() > 0) && ((Serial.peek() == ' ') || (Serial.peek() == '\n') || (Serial.peek() == '\r') || (Serial.peek() == '\0'))) {
      Serial.read();
    }
  }
}

//Check if a new command has been received to the input buffer and respond accordingly.
void serial_respond() {
  if((new_input == true) && (strcmp(input_buffer, "MEAS") == 0)){
    serial_print_sensor_data();
    new_input = false;
  } else if(new_input == true) {
    Serial.print("# Invalid command: ");
    Serial.println(input_buffer);
    new_input = false;
  }
}

//Print all sensor values to the serial interface.
void serial_print_sensor_data()
{
  char output_buffer[128];
  char h_buffer[16];
  char c_buffer[16];
  ftoa(h_buffer, humidity, 2);
  ftoa(c_buffer, temperature_celsius, 2);
  
  sprintf(output_buffer, "{\"humidity\": %s, \"ambient_temperature\": %s}", h_buffer, c_buffer);
  
  Serial.println(output_buffer);
}

//Convert float to string.
char *ftoa(char *buf, double value, int precision)
{
  long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};
 
  char *result = buf;
  long rounded = (long)value;
  itoa(rounded, buf, 10);
  while (*buf != '\0') buf++;
  *buf++ = '.';
  long decimal = abs((long)((value - rounded) * p[precision]));
  itoa(decimal, buf, 10);
  return result;
}
/*************************************/ 




