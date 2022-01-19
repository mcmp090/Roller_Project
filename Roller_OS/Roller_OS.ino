//We always have to include the library
#include "LedControl.h"
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
/*
 Now we need a LedControl to work with.
 ***** These pin numbers will probably not work with your hardware *****
 pin 12 is connected to the DataIn 
 pin 11 is connected to the CLK 
 pin 10 is connected to LOAD 
 We have only a single MAX72XX.
 */
LedControl lc=LedControl(12,11,10,1);
ESP8266WebServer server(80);

/* we always wait a bit between updates of the display */
unsigned long delaytime=100;
double  time1, time2;
float distance = 0, trip = 0; 
byte c;
int topspeed = 0;
int speed1,speed2;
long starttime,endtime, dottime = 0;
bool state = false, firsttime = true, alarmstate = false, lockstate = false;
int16_t temperature;

const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data

int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data

int16_t start_x, start_y, start_z; // start data

int16_t x, y, z; // start data

char tmp_str[7]; // temporary variable used in convert function

char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

void setup() {
  EEPROM.begin(512);
  Wire.begin(); // join i2c bus (address optional for master)
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
  pinMode(9,INPUT);
  pinMode(8,INPUT); 
  digitalWrite(8,HIGH);
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.softAP("Roller","12345677",1,1);
  Serial.println("");

  // Wait for connection
  
  Serial.println("");
  Serial.print("Connected to ");
 
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.on("/", handleRoot);

  server.on("/lock", handleButton);

  server.on("/back", handleRoot);

  server.begin();
  Serial.println("HTTP server started");
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,8);
  /* and clear the display */
  lc.clearDisplay(0);
  //byte eins[8]={B0110,B1001,B1001,B1001,B1001,B0110,B0000,B0000};
  //lc.setLed(0,2,1,true);
  //delay(200);
  //Serial.println(String(eins[0])+String(eins[1]));
  //lc.setLed(0,3,1,true);
  //lc.setLed(0,7,1,true);
  //delay(500);
  clearNum();
  //intro();
  
  /*for(int i = 0;i < 100; i++){
    writeNum(i);
    delay(500);
  }*/
  
  Wire.beginTransmission(MPU_ADDR);
              Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
              Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
              Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers
  
              // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
              start_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
              start_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
              start_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)

              Serial.print("sX = "); Serial.print(convert_int16_to_str(start_x));
  Serial.print(" | sY = "); Serial.print(convert_int16_to_str(start_y));
  Serial.print(" | sZ = "); Serial.print(convert_int16_to_str(start_z));
  // the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
  Serial.print(" | tmp = "); Serial.print(temperature/340.00+36.53);

  Serial.println();
}

/*
 This method will display the characters for the
 word "Arduino" one after the other on the matrix. 
 (you need at least 5x7 leds to see the whole chars)
 */


void loop() { 
  
  if(digitalRead(8) == LOW && lockstate == false){
    firsttime == true;
    if(state == false){
      state = true;
      trip = 0;
      intro();
    }
    time1 = millis();
    while(digitalRead(9)== HIGH){if((millis()-time1) > 3000){
      break;}}
    while(digitalRead(9)== LOW){}
    time1 = millis();
    //Serial.println(time1);
    while(digitalRead(9)== HIGH){if((millis()-time1) > 3000){
      break;}
    }
    while(digitalRead(9)== LOW){}
    time2 = (millis() - time1);
    Serial.println(time2);
    //Serial.println((500/time2)*3.6);
    speed1 = (500/time2)*3.6;
    distance =  distance + 0.5;
    trip = trip + 0.5;
    if(speed1 > topspeed){
      topspeed = speed1;
    }
    if(speed1 != speed2){
      writeNum(speed1);
      speed2 = speed1;
    }
    //int speed2 = speed1 * 3.6;
    //Serial.println(speed1);
    time1 = 0;
    time2 = 0;
  }else{
    if(state == true){
      double Trip = round(trip);
      double dis;
      EEPROM_readAnything(0,dis);
      EEPROM_writeAnything(0, (Trip + dis));
      outro();
      
      state = false;
    }
    if(lockstate == false){
      server.handleClient();
      Wire.requestFrom(8, 1);    // request 6 bytes from slave device #8

      while (Wire.available()) { // slave may send less than requested
        c = Wire.read(); // receive a byte as character
        Serial.println(c);         // print the character
      }
      if(c == 1){
        c = 0;
        //Serial.println("in loop");
      lc.setIntensity(0,15);
      byte lock[8]={0x18,0x24,0x24,0x24,0x7E,0x7E,0x7E,0x7E};
      for(int i = 0;i < 8;i++){
          lc.setRow(0, i, lock[i]);
        }
        delay(2500);
        lc.clearDisplay(0);
        starttime = millis();
      lockstate = true;
      }
    }
    if(lockstate == true){
    if((millis()-starttime) > 15000 || firsttime == false){
      
          if(firsttime == true){
            firsttime == false;
              Wire.beginTransmission(MPU_ADDR);
              Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
              Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
              Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers
  
              // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
              start_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
              start_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
              start_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
          }
            
            if(millis()-dottime > 4000){
              lc.setLed(0, 7, 7, true);
              delay(60);
              lc.setLed(0, 7, 7, false);
              dottime = millis();
            }
            Wire.beginTransmission(MPU_ADDR);
            Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
            Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
            Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers
  
            // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
            accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
            accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
            accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
            temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
            String stringOne =  String(temperature/340.00+36.53); 

            x = start_x - accelerometer_x;
            x = abs(x);
            y = start_y - accelerometer_y;
            y = abs(y);
            z = start_z - accelerometer_z;
            z = abs(z);
            if((x > 2000) || (y > 2000) || (z > 2000)){

            alarmstate = true;
            Serial.println("Alarm!");
            
            for(int x = 0; x < 5; x++){
            byte full[8]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};  
            for(int i = 0;i < 8;i++){
              lc.setRow(0, i, full[i]);
              }
            for(int i = 2000; i < 2500; i = i+2){
            tone(7, i);
            delay(1);
            }
            noTone(7);
            
            delay(10);
            lc.clearDisplay(0);
            delay(10);
            }
            lc.setLed(0, 7, 0, true);
    
            Wire.beginTransmission(MPU_ADDR);
              Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
              Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
              Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers
  
              // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
              start_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
              start_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
              start_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
            }
            server.handleClient();
            Wire.requestFrom(8, 1);    // request 6 bytes from slave device #8

            while (Wire.available()) { // slave may send less than requested
              c = Wire.read(); // receive a byte as character
              Serial.println(c);         // print the character
            }
            if(c == 1){
              c = 0;
              lc.setIntensity(0,8);
              byte smily[8]={0x00,0x24,0x24,0x24,0x81,0x81,0x66,0x18};
              for(int i = 0;i < 8;i++){
              lc.setRow(0, i, smily[i]);
              }
              delay(1300);
              byte smily1[8]={0x00,0x20,0x2E,0x20,0x81,0x81,0x66,0x18};
              for(int i = 0;i < 8;i++){
              lc.setRow(0, i, smily1[i]);
              }
              delay(300);
              for(int i = 0;i < 8;i++){
              lc.setRow(0, i, smily[i]);
              }
              delay(1500);
              lc.clearDisplay(0);
              
              lockstate = false;
            }
    }
    }
 }
 //delay(100);
}

void clearNum(){
  for(int i = 0;i < 6; i++){
    for(int l = 0;l < 8; l++){
       lc.setLed(0,i,l,false);   
    }
  }
}

void writeNum(int num){
  clearNum();
  if(num < 10){
    printNum(2,num);
  }else{
    int e=num%10; // Einer berechnen
    int j=num/10; // Zehner berechnen
    printNum(1,j);
    printNum(2,e);
  }
}


void printNum(int pos, int val){
  int addi = 0;
  if(pos == 2){
    addi = 4;
  }
    switch (val){
      case 0:
        lc.setLed(0,0,addi + 1,true);
        lc.setLed(0,0,addi + 2,true);
        lc.setLed(0,1,addi + 0,true);
        lc.setLed(0,1,addi + 3,true);
        lc.setLed(0,2,addi + 0,true);
        lc.setLed(0,2,addi + 3,true);
        lc.setLed(0,3,addi + 0,true);
        lc.setLed(0,3,addi + 3,true);
        lc.setLed(0,4,addi + 0,true);
        lc.setLed(0,4,addi + 3,true);
        lc.setLed(0,5,addi + 1,true);
        lc.setLed(0,5,addi + 2,true);
      break;

      case 1:
        lc.setLed(0,0,addi + 2,true);
        lc.setLed(0,1,addi + 1,true);
        lc.setLed(0,1,addi + 2,true);
        lc.setLed(0,2,addi + 0,true);
        lc.setLed(0,2,addi + 2,true);
        lc.setLed(0,3,addi + 2,true);
        lc.setLed(0,4,addi + 2,true);
        lc.setLed(0,5,addi + 2,true);
      break;

      case 2:
        lc.setLed(0,0,addi + 1,true);
        lc.setLed(0,0,addi + 2,true);
        lc.setLed(0,1,addi + 0,true);
        lc.setLed(0,1,addi + 3,true);
        lc.setLed(0,2,addi + 3,true);
        lc.setLed(0,3,addi + 2,true);
        lc.setLed(0,4,addi + 1,true);
        lc.setLed(0,5,addi + 0,true);
        lc.setLed(0,5,addi + 1,true);
        lc.setLed(0,5,addi + 2,true);
        lc.setLed(0,5,addi + 3,true);
      break;

      case 3:
        lc.setLed(0,0,addi + 1,true);
        lc.setLed(0,0,addi + 2,true);
        lc.setLed(0,1,addi + 0,true);
        lc.setLed(0,1,addi + 3,true);
        lc.setLed(0,2,addi + 2,true);
        lc.setLed(0,2,addi + 3,true);
        lc.setLed(0,3,addi + 2,true);
        lc.setLed(0,3,addi + 3,true);
        lc.setLed(0,4,addi + 0,true);
        lc.setLed(0,4,addi + 3,true);
        lc.setLed(0,5,addi + 1,true);
        lc.setLed(0,5,addi + 2,true);
      break;

      case 4:
        lc.setLed(0,0,addi + 0,true);
        lc.setLed(0,0,addi + 3,true);
        lc.setLed(0,1,addi + 0,true);
        lc.setLed(0,1,addi + 3,true);
        lc.setLed(0,2,addi + 0,true);
        lc.setLed(0,2,addi + 1,true);
        lc.setLed(0,2,addi + 2,true);
        lc.setLed(0,2,addi + 3,true);
        lc.setLed(0,3,addi + 3,true);
        lc.setLed(0,4,addi + 3,true);
        lc.setLed(0,5,addi + 3,true);
       break;

       case 5:
        lc.setLed(0,0,addi + 0,true);
        lc.setLed(0,0,addi + 1,true);
        lc.setLed(0,0,addi + 2,true);
        lc.setLed(0,0,addi + 3,true);
        lc.setLed(0,1,addi + 0,true);
        lc.setLed(0,2,addi + 0,true);
        lc.setLed(0,2,addi + 1,true);
        lc.setLed(0,2,addi + 2,true);
        lc.setLed(0,3,addi + 3,true);
        lc.setLed(0,4,addi + 3,true);
        lc.setLed(0,5,addi + 0,true);
        lc.setLed(0,5,addi + 1,true);
        lc.setLed(0,5,addi + 2,true);   
       break;

       case 6:
        lc.setLed(0,0,addi + 1,true);
        lc.setLed(0,0,addi + 2,true);
        lc.setLed(0,0,addi + 3,true);
        lc.setLed(0,1,addi + 0,true);
        lc.setLed(0,2,addi + 0,true);
        lc.setLed(0,2,addi + 1,true);
        lc.setLed(0,2,addi + 2,true);
        lc.setLed(0,3,addi + 0,true);
        lc.setLed(0,3,addi + 3,true);
        lc.setLed(0,4,addi + 0,true);
        lc.setLed(0,4,addi + 3,true);
        lc.setLed(0,5,addi + 1,true);
        lc.setLed(0,5,addi + 2,true);
       break;

       case 7:
        lc.setLed(0,0,addi + 0,true);
        lc.setLed(0,0,addi + 1,true);
        lc.setLed(0,0,addi + 2,true);
        lc.setLed(0,0,addi + 3,true);
        lc.setLed(0,1,addi + 3,true);
        lc.setLed(0,2,addi + 2,true);
        lc.setLed(0,3,addi + 1,true);
        lc.setLed(0,4,addi + 0,true);
        lc.setLed(0,5,addi + 0,true);
       break;

       case 8:
        lc.setLed(0,0,addi + 1,true);
        lc.setLed(0,0,addi + 2,true);
        lc.setLed(0,1,addi + 0,true);
        lc.setLed(0,1,addi + 3,true);
        lc.setLed(0,2,addi + 1,true);
        lc.setLed(0,2,addi + 2,true);
        lc.setLed(0,3,addi + 1,true);
        lc.setLed(0,3,addi + 2,true);
        lc.setLed(0,4,addi + 0,true);
        lc.setLed(0,4,addi + 3,true);
        lc.setLed(0,5,addi + 1,true);
        lc.setLed(0,5,addi + 2,true);
       break;

       case 9:
        lc.setLed(0,0,addi + 1,true);
        lc.setLed(0,0,addi + 2,true);
        lc.setLed(0,1,addi + 0,true);
        lc.setLed(0,1,addi + 3,true);
        lc.setLed(0,2,addi + 0,true);
        lc.setLed(0,2,addi + 3,true);
        lc.setLed(0,3,addi + 1,true);
        lc.setLed(0,3,addi + 2,true);
        lc.setLed(0,3,addi + 3,true);
        lc.setLed(0,4,addi + 3,true);
        lc.setLed(0,5,addi + 0,true);
        lc.setLed(0,5,addi + 1,true);
        lc.setLed(0,5,addi + 2,true);
        break;
    }
}
    void intro(){
      int t;
      byte l[8]={0xFF,0x81,0x81,0x81,0x81,0x81,0x81,0xFF};
      byte x[8]={0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00};
      byte y[8]={0x00,0x00,0x3C,0x24,0x24,0x3C,0x00,0x00};
      byte z[8]={0x00,0x7E,0x42,0x42,0x42,0x42,0x7E,0x00};
      //---------------------------------------------------
      byte a[8]={0x00,0x00,0x00,0x00,0x30,0x60,0xC0,0x80};
      byte c[8]={0x00,0x00,0x00,0x00,0x30,0xE0,0x80,0x00};
      byte d[8]={0x00,0x00,0x00,0x00,0x70,0xE0,0x00,0x00};
      byte f[8]={0x00,0x00,0x00,0xF0,0xF0,0x00,0x00,0x00};
      byte g[8]={0x00,0x00,0xE0,0x70,0x00,0x00,0x00,0x00};
      byte h[8]={0x00,0x80,0xE0,0x30,0x00,0x00,0x00,0x00};
      byte k[8]={0x80,0xC0,0x60,0x30,0x00,0x00,0x00,0x00};
      byte k1[8]={0x60,0x20,0x30,0x10,0x00,0x00,0x00,0x00};
      byte k2[8]={0x20,0x30,0x30,0x10,0x00,0x00,0x00,0x00};
      byte k3[8]={0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00};
      byte k4[8]={0x04,0x0C,0x0C,0x08,0x00,0x00,0x00,0x00};
      byte k5[8]={0x06,0x04,0x0C,0x08,0x00,0x00,0x00,0x00};
      byte k6[8]={0x03,0x06,0x0C,0x08,0x00,0x00,0x00,0x00};
      byte k7[8]={0x00,0x01,0x07,0x0C,0x00,0x00,0x00,0x00};
      byte k8[8]={0x00,0x00,0x07,0x0E,0x00,0x00,0x00,0x00};
      byte k9[8]={0x00,0x00,0x00,0x0F,0x0F,0x00,0x00,0x00};
      byte k10[8]={0x00,0x00,0x00,0x00,0x0E,0x07,0x00,0x00};
      byte k11[8]={0x00,0x00,0x00,0x00,0x0C,0x07,0x01,0x00};
      byte k12[8]={0x00,0x00,0x00,0x00,0x0C,0x06,0x03,0x01};
      //---------------------------------------------------
      byte k13[8]={0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18};
      byte k14[8]={0x00,0x00,0x00,0x00,0x18,0x24,0x24,0x24};
      byte k15[8]={0x00,0x00,0x18,0x24,0x42,0x42,0x42,0x42};
      byte k16[8]={0x18,0x24,0x42,0x81,0x81,0x81,0x81,0x81};
      //---------------------------------------------------
      byte k17[8]={0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18};
      byte k18[8]={0x00,0x00,0x00,0x00,0x18,0x24,0x3C,0x3C};
      byte k19[8]={0x00,0x00,0x18,0x24,0x5A,0x66,0x7E,0x7E};
      byte k20[8]={0x18,0x24,0x5A,0xA5,0xDB,0xE7,0xFF,0xFF};
      //---------------------------------------------------
      int b = 2;
      //---------------------------------------------------
      for(int t1 = 100;t1 > 0;t1 = t1 - 20){
        lc.setIntensity(0,b);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, x[i]);
        }
        delay(t1);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, y[i]);
        }
        delay(t1);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, z[i]);
        }
        delay(t1);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, l[i]);
        }
        delay(t1);
        b = b + 2;
      }
      //---------------------------------------------------
      delay(500);
      t = 50;
      for(int i = 0;i < 8;i++){
          lc.setRow(0, i, a[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, c[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, d[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, f[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, g[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, h[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k1[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k2[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k3[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k4[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k5[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k6[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k7[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k8[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k9[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k10[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k11[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k12[i]);
        }
        delay(t+450);

        //----

        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k12[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k11[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k10[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k9[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k8[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k7[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k6[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k5[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k4[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k3[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k2[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k1[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, h[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, g[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, f[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, d[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, c[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, a[i]);
        }
        delay(t);
        //---------------------------------------------------
        delay(1100);
        /*t = 50;
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k13[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k14[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k15[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k16[i]);
        }
        delay(t);
        //---------------------------------------------------
        delay(500);
        t = 50;
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k17[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k18[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k19[i]);
        }
        delay(t);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, k20[i]);
        }
        delay(t);*/
        lc.clearDisplay(0);
        writeNum(0);
    }

    void outro(){
      byte z[8]={0xFF,0x81,0x81,0x81,0x81,0x81,0x81,0xFF};
      byte y[8]={0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00};
      byte x[8]={0x00,0x00,0x3C,0x24,0x24,0x3C,0x00,0x00};
      byte l[8]={0x00,0x7E,0x42,0x42,0x42,0x42,0x7E,0x00};
      int b = 14;
      //---------------------------------------------------
      for(int t1 = 100;t1 > 0;t1 = t1 - 20){
        lc.setIntensity(0,b);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, x[i]);
        }
        delay(t1);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, y[i]);
        }
        delay(t1);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, z[i]);
        }
        delay(t1);
        for(int i = 0;i < 8;i++){
          lc.setRow(0, i, l[i]);
        }
        delay(t1);
        b = b - 2;
      }
      lc.clearDisplay(0);
    }

void handleRoot() {
  server.send(200, "text/html", "<html><head><title>Roller</title><style>body {background: linear-gradient(to bottom, #00bfff,   #00ee00);}h1 {color: #00ee00; border-color: #00ee00; border-width: 5px; border-style: solid;}</style></head><script type=\"text/javascript\">function zoom() {document.body.style.zoom = \"400%\" }</script><body onload=\"zoom()\"><center><h1>RollerOS</h1></br>Km: " + km() + "</br>Trip: " + String(trip) + "</br>Topspeed: " + String(topspeed) + "</br></br><form action=\"/lock\"><input type=\"submit\" value=\"Lock/Unlock\" /></form></center></body></html>");
}

void handleButton() {
  c = 1;
  server.send(200, "text/html", "<html><head><title>Accepted</title><style>body {background: linear-gradient(to bottom, #00bfff,   #00ee00);}h1 {color:   #00ff00;} p { padding-top: 20%; }</style><meta http-equiv=\"refresh\" content=\"2; URL=/back\"></head><script type=\"text/javascript\">function zoom() {document.body.style.zoom = \"300%\" }</script><body onload=\"zoom()\"><h1><center><p>&#10004;</p></center></h1></body></html>");
  //delay(4000);
  //server.send(200, "text/html", "<html><head><title>Roller</title></head><script type=\"text/javascript\">function zoom() {document.body.style.zoom = \"400%\" }</script><body onload=\"zoom()\"><h1>RollerOS</h1></br><h3>Km: </h3></br><h3>Trip: </h3></br></br></br><form action=\"/lock\"><input type=\"submit\" value=\"Lock/Unlock\" /></form></body></html>");
}
String km(){
  long dis;
  EEPROM_readAnything(0, dis);
  return String(dis);
}

template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    int i;
    for (i = 0; i < sizeof(value); i++)
        EEPROM.write(ee++, *p++);
    return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
    byte* p = (byte*)(void*)&value;
    int i;
    for (i = 0; i < sizeof(value); i++)
        *p++ = EEPROM.read(ee++);
    return i;
}
