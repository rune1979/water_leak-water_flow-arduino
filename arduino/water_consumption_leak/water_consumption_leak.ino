#include <Arduino.h>
#include <SPI.h>
//#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
//#include <SoftwareSerial.h>
//#endif
#include <SD.h>

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"
#define FACTORYRESET_ENABLE         0
#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"
#define SEND_SECOND_PLOT            0
SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN,
                      BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);
// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}
/*
 SD card read/write
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 10 (for MKRZero SD: SDCARD_SS_PIN)
 */
//SD CARD
//#include <SPI.h>

File myFile;
int CS_pin = 10; //Your CS pin

//Termistors
int ThermistorPin1 = 3;
int ThermistorPin2 = 4;
int ThermistorPin3 = 5;
float R01 = 9910;// Manually calibrate each thermister, due to wiring and divertion in Resistors 
float R02 = 9910;
float R03 = 10000;
float term_tolerance = 0.003; // 0.2% depending on product
float tol;

float Voa;
float Vob;
float Voc;
float R1, R2, R3;
float RA1, RA2, RA3; // Avarageing 10 messurements
float LR1 = 0;
float LR11;
float Old_R1;

//General Vars
int fdetect = 0;
int ldetect = 0;
int amount = 0;
int coldhot;
float lastrecord;
float diff;
float stop_register;
float differance_set;
// Conductivity ratio water to air is about 1/24 or aproxemately 4 percent, but other material would also have an effect so we will set it to 10 %
float cond; 
float liter_conv = 0;
float liter = 0;
float alert = 0;

//Timers
unsigned long reg_millis = 0;
unsigned long leak_millis = 0;
unsigned long timer_millis = 0;
unsigned long log_millis = 0;
unsigned long alert_millis = 0;
float time_t = 0;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
//  while (!Serial) {
//    ; // wait for serial port to connect. Needed for native USB port only
//  }
//  Serial.print("Initializing SD card...");
//  
//  // CS PIN in this instance it is 10, but make sure to check your self
//  if (!SD.begin(CS_pin)) {
//    Serial.println("initialization failed!");
//    while (1);
//  }
//  Serial.println("initialization done.");
  
  read_termister(); //Call read function
  // First run, calibrate the max difference for air to pipe temperature, make sure that 
  // the water has not been runing for at least three hours before
  differance_set = abs(R1 - R3);
  R1 = RA1;
  R2 = RA2;
  R3 = RA3;
  LR11 = R1;

  // BLE setup
  delay(500);
  if ( !ble.begin(VERBOSE_MODE) )
  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  if ( FACTORYRESET_ENABLE )
  {
    /* Perform a factory reset to make sure everything is in a known state */
    Serial.println(F("Performing a factory reset: "));
    if ( ! ble.factoryReset() ) {
      error(F("Couldn't factory reset"));
    }
  }

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  /* Print Bluefruit information */
  ble.info();

  Serial.println(F("Please use Adafruit Bluefruit LE app to connect in UART mode"));
  Serial.println(F("Then Enter characters to send to Bluefruit"));
  Serial.println();

  ble.verbose(false);  // debug info is a little annoying after this point!

  /* Wait for connection */
  while (! ble.isConnected()) {
    delay(500);
}

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Set module to DATA mode
  Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);
}

void loop() {
  
  if (fdetect == 0 and millis() - 700 > reg_millis) {
    read_termister(); // Get reading
   
    Serial.println("tol: " + String(tol) + ", cond: " + String(cond) + ", diff: " + String(diff) + ", R1: " + String(R1) + ", R2: " + String(R2) + ", R3: " + String(R3) + ", LR11: " + String(LR11));
    if (abs(LR11 - R1) > tol and millis() > 2000){
        fdetect = 1;       
        
        if (LR11 < R1){
          coldhot = 1;
          Serial.println("fdetect cold!");
          LR1 = R2 + abs(diff);
          }
        else{
          coldhot = 0;
          Serial.println("fdetect hot!");
          LR1 = R2 - abs(diff);
          }
        }
    
    reg_millis = millis();
    }

  if (fdetect == 1 and millis() - 150 > log_millis){
    read_termister(); // Get reading
   
    if (coldhot == 1){ // 1 = Cold = rise in resistance 
      if (R2 > LR1){ // We expect this value to be positiv, but it may not nessesarily be. So, we make it absolut.
        Serial.println("Amount:" + String(amount) + ", R2: " + String(R2) + ", LR1: " + String(LR1) + ", R1: " + String(R1));
        amount = amount + 1;
        LR1 = R2 + abs(diff); // Next level to be reached to make a count     
        }
        if (millis() - 120000 > timer_millis){ 
           Old_R1 = R1 - tol; // If R1 is returning to ambient, the water has stoped running. Cancels out any tolerance deviation
           timer_millis = millis();
           }
        if (Old_R1 > R1){
           fdetect = 0;
           Serial.println("Detection stoped");
           }
    }
    else if (coldhot == 0){
      if (R2 < LR1){
        Serial.println("Amount added R2 < lastrecord, Amount:" + String(amount) + ", R2: " + String(R2) + ", LR1: " + String(LR1) + ", R1: " + String(R1));
        amount = amount + 1;
        LR1 = R2 - abs(diff);
      }
      if (millis() - 120000 > timer_millis){ 
           Old_R1 = R1 + tol; // If R1 is returning to ambient, the water has stoped running. Cancels out any tolerance deviation
           timer_millis = millis();
      }
      if (Old_R1 < R1){
           fdetect = 0;
           Serial.println("Detection stoped");
           }
        
      }
    log_millis = millis();
  }
  
  if (millis() > leak_millis + 14000){
    read_termister(); // Get reading
    LR11 = R1;
    if (abs(R1 - R3) < differance_set){
      alert_millis = millis();
      }
    if (alert_millis + 300000 < millis()){
      Serial.println("Leak detected");
      alert = 1;
      }
    leak_millis = millis();
    }

if (millis() > 180000 and millis() < 185000){
  liter_conv = 5 / amount;
   
  }

// Write to file and Bluetooth every 5 seconds
if (millis() > time_t + 5000){
  Serial.println(String(R1) + "," + String(liter) + "," + String(alert));
//  myFile = SD.open("test6.txt", FILE_WRITE);
//  if (myFile) {
//      myFile.println("R1 (input): " + String(R1) + ", R2 (output): " + String(R2) + ", Amount: " + String(amount) + ", time: " + String(millis()));
//      // close the file:
//      myFile.close();
//
//      // BLE Write
//     
//      
//    }
  liter = liter_conv * amount;
  time_t = millis();
  ble.print(String(liter) + "," + String(alert));
  }  
}
