
#include <IRremote.h>
#include <IRremoteInt.h>
#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <printf.h>
#include <EEPROM.h>
#include "Keeloq.h"

#define DEBUG

#define TMP102_I2C_ADDRESS 72
#define TIMEOUT_MILLIS 100L
#define LED_PIN 8

#define LOC_SEQNUM 0x00
#define LOC_KEY_LO 0x04
#define LOC_KEY_HI 0x08

RF24 radio(9,10);
IRsend irsend;
float correctedtemp;
unsigned long message;
uint32_t encSequenceNumber;
uint32_t decSequenceNumber;
uint32_t sequenceNumber;
Keeloq *k;
int8_t firstbyte, secondbyte;
uint8_t buffer[10];
String s_key_lo, s_key_hi, s_seq;
uint32_t key_lo, key_hi;

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600); 

  key_lo = ((uint32_t)EEPROM.read(LOC_KEY_LO) << 24) | ((uint32_t)EEPROM.read(LOC_KEY_LO+1) << 16) | 
      ((uint32_t)EEPROM.read(LOC_KEY_LO+2) << 8) | ((uint32_t)EEPROM.read(LOC_KEY_LO+3));
  key_hi = ((uint32_t)EEPROM.read(LOC_KEY_HI) << 24) | ((uint32_t)EEPROM.read(LOC_KEY_HI+1) << 16) | 
      ((uint32_t)EEPROM.read(LOC_KEY_HI+2) << 8) | ((uint32_t)EEPROM.read(LOC_KEY_HI+3));
#ifdef DEBUG
  Serial.print(F("key_lo=")); Serial.println(key_lo, HEX);
  Serial.print(F("key_hi=")); Serial.println(key_hi, HEX);
#endif
  sequenceNumber = ((uint32_t)EEPROM.read(LOC_SEQNUM) << 24) | ((uint32_t)EEPROM.read(LOC_SEQNUM+1) << 16) | 
        ((uint32_t)EEPROM.read(LOC_SEQNUM+2) << 8) | ((uint32_t)EEPROM.read(LOC_SEQNUM+3));
#ifdef DEBUG
  Serial.print(F("seq=")); Serial.println(sequenceNumber);
#endif

  k = new Keeloq(key_lo, key_hi);
  printf_begin();
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
  Serial.print(F("TMP102 CFG ")); Serial.print(firstbyte, HEX); Serial.print(F(" ")); Serial.println(secondbyte, HEX);
#endif
  radio.begin();
  radio.setRetries(15,15);
  //radio.setDataRate(RF24_250KBPS);
  //radio.setPayloadSize(5);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1,pipes[0]);
  radio.startListening();
#ifdef DEBUG
  radio.printDetails();
#endif
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
  while (!radio.available() && !Serial.available()) ;
  if (Serial.available()) {
    byte c = Serial.read(); 
    if (c == 'K' || c == 'k') {
      Serial.readStringUntil(' ');
      s_key_lo = Serial.readStringUntil(' ');
      s_key_hi = Serial.readStringUntil(' ');
      //Serial.print(F("key_lo=")); Serial.println(s_key_lo);
      //Serial.print(F("key_hi=")); Serial.println(s_key_hi);
      key_lo = strtoul(s_key_lo.c_str(),NULL,16);
      Serial.print(F("key_lo_parsed=")); Serial.println(key_lo, HEX);
      key_hi = strtoul(s_key_hi.c_str(),NULL,16);
      Serial.print(F("key_lo_parsed=")); Serial.println(key_hi, HEX);
      if (k) delete k;
      k = new Keeloq(key_lo, key_hi);
      EEPROM.write(LOC_KEY_LO,(uint8_t)((key_lo & 0xff000000) >> 24));
      EEPROM.write(LOC_KEY_LO+1,(uint8_t)((key_lo & 0x00ff0000) >> 16));
      EEPROM.write(LOC_KEY_LO+2,(uint8_t)((key_lo & 0x0000ff00) >> 8));
      EEPROM.write(LOC_KEY_LO+3,(uint8_t)((key_lo & 0x000000ff)));
      EEPROM.write(LOC_KEY_HI,(uint8_t)((key_hi & 0xff000000) >> 24));
      EEPROM.write(LOC_KEY_HI+1,(uint8_t)((key_hi & 0x00ff0000) >> 16));
      EEPROM.write(LOC_KEY_HI+2,(uint8_t)((key_hi & 0x0000ff00) >> 8));
      EEPROM.write(LOC_KEY_HI+3,(uint8_t)((key_hi & 0x000000ff)));
    } else if (c == 'S' || c == 's') {
      Serial.readStringUntil(' ');
      s_seq = Serial.readStringUntil(' ');
      //Serial.print(F("seq=")); Serial.println(s_seq);
      sequenceNumber = strtoul(s_seq.c_str(),NULL,10);
      Serial.print(F("seq_parsed=")); Serial.println(sequenceNumber);
      EEPROM.write(LOC_SEQNUM,(uint8_t)((sequenceNumber & 0xff000000) >> 24));
      EEPROM.write(LOC_SEQNUM+1,(uint8_t)((sequenceNumber & 0x00ff0000) >> 16));
      EEPROM.write(LOC_SEQNUM+2,(uint8_t)((sequenceNumber & 0x0000ff00) >> 8));
      EEPROM.write(LOC_SEQNUM+3,(uint8_t)((sequenceNumber & 0x000000ff)));
    }
  } 
  if (radio.available()) {
    radio.read(buffer, 10);
    Serial.print((int)buffer[0]); 
  #ifdef DEBUG
    Serial.print(F(" - ")); Serial.print((int)buffer[1],HEX);
    Serial.print(F(" - ")); Serial.print((int)buffer[2],HEX); 
    Serial.print(F(" - ")); Serial.print((int)buffer[3],HEX);
    Serial.print(F(" - ")); Serial.print((int)buffer[4],HEX);
  #endif
    Serial.println();
    if (buffer[0] == 1) {
      message = ((uint32_t)buffer[1] << 24) | ((uint32_t)buffer[2] << 16) | 
        ((uint32_t)buffer[3] << 8) | ((uint32_t)buffer[4]);
      encSequenceNumber = ((uint32_t)buffer[5] << 24) | ((uint32_t)buffer[6] << 16) | 
        ((uint32_t)buffer[7] << 8) | ((uint32_t)buffer[8]);
      decSequenceNumber = k->decrypt(encSequenceNumber);
      sequenceNumber = ((uint32_t)EEPROM.read(LOC_SEQNUM) << 24) | ((uint32_t)EEPROM.read(LOC_SEQNUM+1) << 16) | 
        ((uint32_t)EEPROM.read(LOC_SEQNUM+2) << 8) | ((uint32_t)EEPROM.read(LOC_SEQNUM+3));
      if (decSequenceNumber >= sequenceNumber && decSequenceNumber < sequenceNumber+100) {
  #ifdef DEBUG      
        Serial.print(F("Sequence valid: internal=")); Serial.print(sequenceNumber); Serial.print(F(" received=")); Serial.println(decSequenceNumber);
  #endif
        sequenceNumber = decSequenceNumber + 1;
        EEPROM.write(LOC_SEQNUM,(uint8_t)((sequenceNumber & 0xff000000) >> 24));
        EEPROM.write(LOC_SEQNUM+1,(uint8_t)((sequenceNumber & 0x00ff0000) >> 16));
        EEPROM.write(LOC_SEQNUM+2,(uint8_t)((sequenceNumber & 0x0000ff00) >> 8));
        EEPROM.write(LOC_SEQNUM+3,(uint8_t)((sequenceNumber & 0x000000ff)));
        //Serial.println(message, HEX);
        irsend.sendWhynter(message,32);
        if (message == 0x8c002aa5) 
          digitalWrite(LED_PIN, 1);
        else
          digitalWrite(LED_PIN, 0);
        } else {
  #ifdef DEBUG        
          Serial.print(F("Sequence mismatch: internal=")); Serial.print(sequenceNumber); Serial.print(F(" received=")); Serial.println(decSequenceNumber);
  #endif
        }
    } else if (buffer[0] == 2) {
      radio.stopListening();
      digitalWrite(LED_PIN, 1);
      delay(100);
      digitalWrite(LED_PIN,0);
      getTemp102();
      buffer[0] = firstbyte;
      buffer[1] = secondbyte;
      buffer[2] = 0xAA;
      buffer[3] = 0xAA;
      buffer[4] = 0xAA;
      //radio.data[0]=(uint16_t)(correctedtemp*10) >> 8;
      //radio.data[1]=(uint16_t)(correctedtemp*10) & 0x00ff;
  #ifdef DEBUG
       Serial.print(F("RTMP ")); Serial.print(buffer[0], HEX); Serial.print(' '); Serial.println(buffer[1], HEX);
  #endif
      radio.write(buffer, 5);
      radio.startListening();
    }
  }
}
