/*
  LoRa Duplex communication wth callback

  Sends a message every half second, and uses callback
  for new incoming messages. Implements a one-byte addressing scheme,
  with 0xFF as the broadcast address.

*/

/* Include for timer interrupt library */
#include "RPi_Pico_TimerInterrupt.h"

#include <SPI.h>              // include libraries
#include <LoRa.h>

RPI_PICO_Timer Blink_led(0);  // one interrupt timer for the led/buzzer

const int _MISO = 16;
const int _MOSI = 19;
const int _SCK = 18;


const int csPin = 17;         // LoRa radio chip select
const int resetPin = 20;      // LoRa radio reset
const int irqPin = 21;        // change for your board; must be a hardware interrupt pin

String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0xBB;     // address of this device
byte destination = 0xAA;      // destination to send to
long lastSendTime = 0;        // last send time
int interval = 2000;          // interval between sends

void setup() {
  // store 1 byte from uart...
  int incomingByte = 0;   

  pinMode(LED_BUILTIN, OUTPUT);     // led pin output

  if (Blink_led.attachInterruptInterval(500 * 1000, Blink_led_Handler)){
    Serial.println("TIMER: Starting Blink_led");
  } else {
    Serial.println("TIMER: Can't set Blink_led");
  }
  
  Serial.begin(9600);                   // initialize serial
  while (!Serial);

  // wait for receiving a byte from uart
  //while(!Serial.available() > 0);
  //incomingByte = Serial.read();

  //Serial.println("PRESS ENTER!");

  // wait for the "Enter" key
  //while(!(incomingByte == '\n'));

  //Serial.println("LoRa Duplex with callback");

  // override the default CS, reset, and IRQ pins (optional)
  LoRa.setPins(csPin, resetPin, irqPin);// set CS, reset, IRQ pin

  if (!LoRa.begin(915E6)) {             // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                       // if failed, do nothing
  }

  LoRa.onReceive(onReceive);
  LoRa.receive();
  Serial.println("LoRa init succeeded.");
}

void loop() {
  if (millis() - lastSendTime > interval) {
    //String message = "HeLoRa World FROM 0xBB!";   // send a message
    //sendMessage(message);
    //Serial.println("Sending " + message);
    lastSendTime = millis();            // timestamp the message
    //interval = random(2000) + 1000;     // 2-3 seconds
    LoRa.receive();                     // go back into receive mode
  }
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";                 // payload of packet

  while (LoRa.available()) {            // can't use readString() in callback, so
    incoming += (char)LoRa.read();      // add bytes one by one
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message ID: " + String(incomingMsgId));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: ");
  Serial.println(incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
}

bool Blink_led_Handler(struct repeating_timer *t)
{
  (void) t;
  
  static bool ledtoggle = false;

  digitalWrite(LED_BUILTIN, ledtoggle);
  ledtoggle = !ledtoggle;

  return true;
}