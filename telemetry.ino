/* Include for timer interrupt library */
#include "RPi_Pico_TimerInterrupt.h"
/* Includes for BMP280 library */
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
/* Includes for LoRa library */
#include <SPI.h>
#include <LoRa.h>
/* Include for playing melody */
#include "pitches.h"

/*****************************************************************
 *           put your defines here                               *
 *****************************************************************/
#define SAMPLING_INTERVAL   1000 * 6 * 10 // * 240 * 1           // 5 min (300 seconds) sampling the altitude values
#define SAMPLING_PERIOD     15                      // take and store a sample every x ms (this is the maximum sampling rate we can get according to BMP280 settings)
                                                    // at this rate we can get 66.6 samples/s , 4000 samples/min  

#define LED_BUZZER_PERIOD   500                     // toggle led every 500 ms after the initialization so we know everything is ok
#define LED_INTERVAL        1000 * 10 * 1           // after turn on, wait for 1000 ms x 60 seconds x 2 before start the sampling
#define BUZZER_TONE         1000
#define TUNE_LOOP           1

#define DEBUG               1                       // 0 - debug disable, final version to put inside the rocket; 1 - debug version, to try with PC

#if (DEBUG > 0)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINT_LN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINT_LN(x)   
#endif

#define BUZZER_PIN          15
#define SEALEVELPRESSURE_HPA (1023)                   // ADJUST TO YOUR CURRENT LOCATION!! CHECK https://zoom.earth/maps/pressure/

/*****************************************************************
 *           put your variables here                             *
 *****************************************************************/
Adafruit_BMP280 bmp280;                             // use I2C default interface
//File logFile;                                       // create a file to interface the write/read oeprations to/from sdcard
RPI_PICO_Timer Timer_led_buzzer(0);                 // one interrupt timer for the led/buzzer
RPI_PICO_Timer Timer_bmp_sampling(1);               // one interrupt timer for the samplign period

// Configure SPI pins for SD card Interface
const int _MISO = 16;
const int _MOSI = 19;
//const int _CS = 17; SD Card Initializer
const int _SCK = 18;
// Configure Pins for LoRa
const int csPin = 22;          // LoRa radio chip select
const int resetPin = 20;       // LoRa radio reset
const int irqPin = 21;         // change for your board; must be a hardware interrupt pin

// some flags...
static bool buzzer = false;
static bool sampling = false;                         // set to true after LED_INTERVAL
static bool take_sample_now = false;                // triggered every 10 ms by Timer_bmp_sampling

byte msgCount = 0;                                  // Message counter
byte localAddress = 0xAA;                           // Address of this device
byte destination = 0xBB;                            // Destination address for LoRa communication

/*****************************************************************
 *           put your func. prototypes here                      *
 *****************************************************************/
void init_uart(int baudrate);
void init_gpio(void);
void init_timer(void);
void init_sdcard(void);
void init_bpm280(void);
void play_tune(void);
void sendMessage(String message);
bool Timer_ledbuzzer_Handler(struct repeating_timer *t);
bool Timer_bmp_sampling_Handler(struct repeating_timer *t);

/*****************************************************************
 *           setup() code                                        *
 *****************************************************************/
void setup(){

#if (DEBUG > 0)
  init_uart(115200);
#endif  
  init_gpio();
  init_timer();
  init_lora();
  init_bpm280();
}

/*****************************************************************
 *           main loop() code                                    *
 *****************************************************************/
void loop(){
  static unsigned long time_now;
  int int_status;
  int sample = 0;

// code for the setup before launch
  DEBUG_PRINT("Current time: ");
  DEBUG_PRINT_LN(millis());
  DEBUG_PRINT_LN();

  time_now = millis();
  while(millis() - time_now < LED_INTERVAL);

  // loop for sampling
  time_now = millis();  
  DEBUG_PRINT("Current time: ");
  DEBUG_PRINT_LN(time_now);
  
  buzzer = true;
  sampling = true;

  Timer_bmp_sampling.setInterval(SAMPLING_PERIOD * 1000, Timer_bpm280sampling_Handler);

  while (millis() - time_now < SAMPLING_INTERVAL) {
    if (take_sample_now) {
      take_sample_now = false;

      // Read BMP280 sensor data
      float temperature = bmp280.readTemperature();
      float altitude = bmp280.readAltitude(SEALEVELPRESSURE_HPA);

      // Create a message with the data
      String message = String(sample++) + "," + String(millis()) + "," + String(temperature) + "," + String(altitude);
      
      // Send data via LoRa
      sendMessage(message);

      DEBUG_PRINT_LN("Sending via LoRa: ");
      DEBUG_PRINT_LN(message);
    }
  }
  
  Timer_led_buzzer.disableTimer();
  Timer_bmp_sampling.disableTimer();
  
  buzzer = false;
  sampling = false;

  DEBUG_PRINT("Sampling duration (s): ");
  DEBUG_PRINT_LN((millis() - time_now) / 1000); 
  DEBUG_PRINT_LN("Sampling finished! File closed and it is safe to turn off device...");

  // finish sampling! play glorious melody if you want
  play_tune();
  
  while(1);

}

/*****************************************************************
 *            Handler for Timer_ledbuzzer_Handler                *
 *****************************************************************/
bool Timer_ledbuzzer_Handler(struct repeating_timer *t)
{
  (void) t;
  
  static bool ledtoggle = false;

  digitalWrite(LED_BUILTIN, ledtoggle);
  ledtoggle = !ledtoggle;

  if(!sampling){
    DEBUG_PRINT("Led toggle at: ");
    DEBUG_PRINT_LN(millis());
  }

  if(buzzer){
    if(ledtoggle){
      tone(BUZZER_PIN, BUZZER_TONE);
    } else {
      noTone(BUZZER_PIN);
      }    
    }

  return true;
}

/*****************************************************************
 *            Handler for Timer_bpm280_sampling                  *
 *****************************************************************/
bool Timer_bpm280sampling_Handler(struct repeating_timer *t)
{
  (void) t;

  take_sample_now = true;
  
  return true;
}

/*****************************************************************
 *            init_uart() code                                   *
 *****************************************************************/
#if (DEBUG > 0)
void init_uart(int baudrate){

  // store 1 byte from uart...
  int incomingByte = 0;                     

  Serial.begin(115200);
  delay(1000);
  
  // wait for receiving a byte from uart
  while(!Serial.available() > 0);
  incomingByte = Serial.read();

  // wait for the "Enter" key
  while(!(incomingByte == '\n'));

  // print some info to the uart
  DEBUG_PRINT_LN("\nStarting the Altimeter Project...");
}
#endif

/*****************************************************************
 *            init_gpio() code                                   *
 *****************************************************************/
void init_gpio(void){
  pinMode(LED_BUILTIN, OUTPUT);     // led pin output
  pinMode(BUZZER_PIN, OUTPUT);      // buzzer pin output

  DEBUG_PRINT_LN("GPIO: led and buzzer pins OK...");
}

/*****************************************************************
 *            init_timer() code                                  *
 *****************************************************************/
void init_timer(void){

  if (Timer_led_buzzer.attachInterruptInterval(LED_BUZZER_PERIOD * 1000, Timer_ledbuzzer_Handler)){
    DEBUG_PRINT_LN("TIMER: Starting Timer_led_buzzer");
  } else {
    DEBUG_PRINT_LN("TIMER: Can't set Timer_led_buzzer");
  }

  if (Timer_bmp_sampling.attachInterruptInterval(10 * SAMPLING_PERIOD * 1000, Timer_bpm280sampling_Handler)){
    DEBUG_PRINT_LN("TIMER: Starting Timer_bmp_sampling");
  } else {
    DEBUG_PRINT_LN("TIMER: Can't set Timer_bmp_sampling");
  }

  DEBUG_PRINT("Timer: Current time: ");
  DEBUG_PRINT_LN(millis());

}

/*****************************************************************
 *            init_bpm280() code                                 *
 *****************************************************************/
void init_bpm280(void){
    if (!bmp280.begin(0x76)) {
    DEBUG_PRINT_LN("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  bmp280.setSampling(Adafruit_BMP280::MODE_NORMAL, 
                     Adafruit_BMP280::SAMPLING_X1, 
                     Adafruit_BMP280::SAMPLING_X4, 
                     Adafruit_BMP280::FILTER_X16, 
                     Adafruit_BMP280::STANDBY_MS_1);

  DEBUG_PRINT_LN("BMP280: Initialization OK...");
}

/*****************************************************************
 *            init_lora() code                                   *
 *****************************************************************/
void init_lora(void) {
  LoRa.setPins(csPin, resetPin, irqPin);

  if (!LoRa.begin(915E6)) {
    DEBUG_PRINT_LN("LoRa init failed. Check your connections.");
    while (1);
  }
  DEBUG_PRINT_LN("LoRa: Initialization OK...");
}

/*****************************************************************
 *            sendMessage() code                                 *
 *****************************************************************/
void sendMessage(String message) {
  LoRa.beginPacket();
  LoRa.write(destination);
  LoRa.write(localAddress);
  LoRa.write(msgCount++);
  LoRa.write(message.length());
  LoRa.print(message);
  LoRa.endPacket();
}

/*****************************************************************
 *            play final tune                                    *
 *****************************************************************/
void play_tune(void){

int melody[] = {NOTE_AS4, NOTE_AS4, NOTE_AS4, NOTE_F5, NOTE_C6, NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F6, NOTE_C6, NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F6, NOTE_C6, NOTE_AS5, 
  NOTE_A5, NOTE_AS5, NOTE_G5, NOTE_C5, NOTE_C5, NOTE_C5, NOTE_F5, NOTE_C6, NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F6, NOTE_C6, NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F6, NOTE_C6, 
  NOTE_AS5, NOTE_A5, NOTE_AS5, NOTE_G5, NOTE_C5, NOTE_C5, NOTE_D5, NOTE_D5, NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_G5, NOTE_D5, NOTE_E5, 
  NOTE_C5, NOTE_C5, NOTE_D5, NOTE_D5, NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F5, NOTE_C6, NOTE_G5, NOTE_G5, 0, NOTE_C5, NOTE_D5, NOTE_D5, NOTE_AS5, NOTE_A5, NOTE_G5, NOTE_F5,
  NOTE_F5, NOTE_G5, NOTE_A5, NOTE_G5, NOTE_D5, NOTE_E5, NOTE_C6, NOTE_C6, NOTE_F6, NOTE_DS6, NOTE_CS6, NOTE_C6, NOTE_AS5, NOTE_GS5, NOTE_G5, NOTE_F5, NOTE_C6
};

int durations[] = {
  8, 8, 8, 2, 2, 8, 8, 8, 2, 4, 8, 8, 8, 2, 4, 8, 8, 8, 2, 8, 8, 8, 2, 2, 8, 8, 8, 2, 4, 8, 8, 8, 2, 4, 8, 8, 8, 2, 8, 16, 4, 8, 8, 8, 8,  8, 8, 8, 8, 4, 8, 4, 8, 
  16, 4, 8, 8, 8, 8, 8, 8, 16, 2, 8, 8, 4, 8, 8, 8, 8, 8, 8, 8, 8, 4, 8, 4, 8, 16, 4, 8, 4, 8, 4, 8, 4, 8, 1
};

int size = sizeof(durations) / sizeof(int);

  for (int i = 0; i < TUNE_LOOP; i++){
    for (int note = 0; note < size; note++) {
      //to calculate the note duration, take one second divided by the note type.
      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      int duration = 1000 / durations[note];
      tone(BUZZER_PIN, melody[note], duration);

      //to distinguish the notes, set a minimum time between them.
      //the note's duration + 30% seems to work well:
      int pauseBetweenNotes = duration * 1.30;
      delay(pauseBetweenNotes);

      //stop the tone playing:
      noTone(BUZZER_PIN);
    }
  }
}
