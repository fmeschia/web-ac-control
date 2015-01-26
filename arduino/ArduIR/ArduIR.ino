
#include <IRremote.h>
#include <IRremoteInt.h>
#include <Wire.h>
#include "Nrf2401.h"

Nrf2401 radio;
IRsend irsend;
float correctedtemp;
unsigned long message;
uint8_t firstbyte, secondbyte;
  
#define TMP102_I2C_ADDRESS 72
#define TIMEOUT_MILLIS 100L
#define DEBUG

void setup()
{
  pinMode(13, OUTPUT);
  Serial.begin(9600); 
  Wire.begin();
  Wire.beginTransmission(TMP102_I2C_ADDRESS);
  Wire.write(0x01); // write to config registry
  Wire.write(0x01); // shutdown mode
  Wire.write(0x00); 
  Wire.endTransmission();
#ifdef DEBUG
  Wire.beginTransmission(TMP102_I2C_ADDRESS);
  Wire.write(0x01);
  Wire.requestFrom(TMP102_I2C_ADDRESS, 2);
  byte firstbyte      = (Wire.read()); 
  byte secondbyte     = (Wire.read());
    Serial.print(F("CFG ")); Serial.print(firstbyte, HEX); Serial.print(F(" ")); Serial.println(secondbyte, HEX);
#endif
  radio.localAddress = 0xAAAA;
  radio.remoteAddress = 0xCCCC;
  radio.power = 3;
  radio.dataRate = 0;
  radio.rxMode(5);
}

void getTemp102(){
  //uint8_t firstbyte, secondbyte;
  Wire.beginTransmission(TMP102_I2C_ADDRESS);
  Wire.write(0x01); // write to config register
  Wire.write(0x80); // one-shot conversion
  Wire.write(0x00); 
  Wire.endTransmission();
  Wire.beginTransmission(TMP102_I2C_ADDRESS);
  Wire.write(0x00); // read temp register
  Wire.endTransmission();
  delay(30); // allow for conversion time
  Wire.requestFrom(TMP102_I2C_ADDRESS, 2);
  boolean timeout = false;
  unsigned long t0 = millis();
  do {
    timeout = ((millis() - t0) > TIMEOUT_MILLIS);
  } while (Wire.available() != 2 && !timeout);
  if (!timeout) {
    firstbyte      = (Wire.read()); 
    secondbyte     = (Wire.read());
#ifdef DEBUG
     Serial.print(F("TMP ")); Serial.print(firstbyte, HEX); Serial.print(' '); Serial.println(secondbyte, HEX);
    //uint16_t temp = ((uint16_t)firstbyte) << 4 | ((uint16_t)secondbyte) >> 4;
    //firstbyte = (temp & 0xff00) >> 8;
    //secondbyte = (temp & 0x00ff);
#endif
    //correctedtemp = (((uint16_t)(firstbyte) << 4)|((uint16_t)(secondbyte) >> 4)) * 0.0625;
  } else {
    firstbyte = 0xff; secondbyte = 0xff;
  }
}

void loop() {
  while (!radio.available());
  radio.read();
  Serial.print((int)radio.data[0]); 
#ifdef DEBUG
  Serial.print(F(" - ")); Serial.print((int)radio.data[1],HEX);
  Serial.print(F(" - ")); Serial.print((int)radio.data[2],HEX); 
  Serial.print(F(" - ")); Serial.print((int)radio.data[3],HEX);
  Serial.print(F(" - ")); Serial.print((int)radio.data[4],HEX);
#endif
  Serial.println();
  if (radio.data[0] == 1) {
    message = ((uint32_t)radio.data[1] << 24) | ((uint32_t)radio.data[2] << 16) | 
      ((uint32_t)radio.data[3] << 8) | ((uint32_t)radio.data[4]);
    //Serial.println(message, HEX);
    irsend.sendWhynter(message,32);
    if (message == 0x8c002aa5) 
      digitalWrite(13, 1);
    else
      digitalWrite(13, 0);
  } else if (radio.data[0] == 2) {
    radio.txMode(5);
    digitalWrite(13, 1);
    delay(100);
    digitalWrite(13,0);
    getTemp102();
    radio.data[0] = firstbyte;
    radio.data[1] = secondbyte;
    radio.data[2] = 0xAA;
    radio.data[3] = 0xAA;
    radio.data[4] = 0xAA;
    //radio.data[0]=(uint16_t)(correctedtemp*10) >> 8;
    //radio.data[1]=(uint16_t)(correctedtemp*10) & 0x00ff;
#ifdef DEBUG
     Serial.print(F("RTMP ")); Serial.print(radio.data[0], HEX); Serial.print(' '); Serial.println(radio.data[1], HEX);
#endif
    radio.write();
    radio.rxMode(5);
  }
}