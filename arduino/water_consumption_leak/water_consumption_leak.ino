#include <SPI.h> 

int BLEorSD = 1; // Use either BLE or SD, 1 = BLE, 0 = SD
/*
 * The following is taken from Adafruits BLE Library, you need to configure this to your own module.
 */

#include "BLE.h" // Bluetooth
#include "BluefruitConfig.h" // Bluetooth

/*
 SD card read/write
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 10 (for MKRZero SD: SDCARD_SS_PIN)
 */
//SD CARD
#include <SD.h> //SD CARD LIBRARY
File myFile;
int CS_pin = 3; //Your CS pin

//Termistors vars
int ThermistorPin1 = 3;
int ThermistorPin2 = 4;
int ThermistorPin3 = 5;
float R01 = 9910;// Manually calibrate each thermister, due to wiring and divertion in resistance
float R02 = 9863;
float R03 = 9790;
float term_tolerance = 0.004; // 0.4% depending on product
float tol;
float cc1 = 1.009249522e-03, cc2 = 2.378405444e-04, cc3 = 2.019202697e-07, logR1, logR2, logR3, C1, C2, C3;

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
float amount = 0;
int coldhot;
float lastrecord;
float diff;
float stop_register;
float differance_set;
// Conductivity ratio water to air is about 1/24 or aproxemately 4 percent, but other material would also have an effect so we will set it to 10 %
float cond; 
float liter;
float alert;
float liter_conv;

//Timers
unsigned long reg_millis = 0;
unsigned long leak_millis = 0;
unsigned long timer_millis = 0;
unsigned long log_millis = 0;
unsigned long alert_millis = 0;
unsigned long time_t = 0;

void setup() {
liter = 0;
alert = 0;
liter_conv = 0;

if (BLEorSD == 0){
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.print("Initializing SD card...");
  
  // CS PIN in this instance it is 10, but make sure to check your self
  if (!SD.begin(CS_pin)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
}else{
  Serial.begin(115200);
ble_setup(); 
}
  read_termister(); //Call read function
  // First run, calibrate the max difference for air to pipe temperature, make sure that 
  // the water has not been runing for at least three hours before
  differance_set = abs(R1 - R3);
  R1 = RA1;
  R2 = RA2;
  R3 = RA3;
  LR11 = R1;

}

void loop() {
  
  if (fdetect == 0 and millis() - 700 > reg_millis) {
    read_termister(); // Get reading
    Serial.println("tol: " + String(tol) + ", diff: " + String(diff) + ", R1: " + String(R1) + ", R2: " + String(R2) + ", R3: " + String(R3) + ", LR11: " + String(LR11));
    temp_conv();
    Serial.println("C1: " + String(C1) + ", C2: " + String(C2) + ", C3: " + String(C3));
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
  
  if (millis() > leak_millis + 10000){
    read_termister(); // Get reading
    LR11 = R1;
    if (abs(R1 - R3) < differance_set){
      alert_millis = millis();
      }
    if (alert_millis + 300000 < millis()){
      Serial.println("Leak detected");
      alert = 15;
      }
    leak_millis = millis();
    }

// Write to file and Bluetooth every 5 seconds
if (millis() > time_t + 5000){
  Serial.println(String(C1) + "," + String(C2) + "," + String(C3) + "," + String(R1) + "," + String(liter) + "," + String(alert) + "," + String(amount) + "," + liter_conv);
  if (BLEorSD == 0){
  myFile = SD.open("test14.txt", FILE_WRITE);
  if (myFile) {
      myFile.println(String(R1) + "," + String(R2) + "," + String(R3) + "," + String(millis()) + "," + String(liter) + "," + String(alert) + "," + String(C1) + "," + String(C2) + "," + String(C3));
      // close the file:
      myFile.close();
      // BLE Write
    }}else{
  ble.print(String(C2) + "," + String(liter) + "," + String(alert));
  //ble.println();
    }
  liter = liter_conv * amount;
  time_t = millis();
 
  }  
}
