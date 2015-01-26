
#include <IRremote.h>
#include <IRremoteInt.h>
#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <printf.h>

RF24 radio(9,10);
IRsend irsend;
float correctedtemp;
unsigned long message;
uint8_t firstbyte, secondbyte;
uint8_t buffer[6];

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

#define DEBUG

#define TMP102_I2C_ADDRESS 72
#define TIMEOUT_MILLIS 100L
#define LED_PIN 8

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600); 
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
    Serial.print(F("CFG ")); Serial.print(firstbyte, HEX); Serial.print(F(" ")); Serial.println(secondbyte, HEX);
#endif
  radio.begin();
  radio.setRetries(15,15);
  //radio.setDataRate(RF24_250KBPS);
  radio.setPayloadSize(5);
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
  while (!radio.available());
  radio.read(buffer, 5);
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
    //Serial.println(message, HEX);
    irsend.sendWhynter(message,32);
    if (message == 0x8c002aa5) 
      digitalWrite(LED_PIN, 1);
    else
      digitalWrite(LED_PIN, 0);
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
