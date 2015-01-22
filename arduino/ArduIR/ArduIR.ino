
#include <IRremote.h>
#include <IRremoteInt.h>
#include <Wire.h>
#include "Nrf2401.h"

Nrf2401 radio;
IRsend irsend;
float correctedtemp;
unsigned long message;
  
#define TMP102_I2C_ADDRESS 72
#define TIMEOUT_MILLIS 100L

void setup()
{
  pinMode(13, OUTPUT);
  Serial.begin(9600); 
  Wire.begin();
  radio.localAddress = 1;
  radio.remoteAddress = 2;
  radio.power = 3;
  radio.dataRate = 0;
  radio.rxMode(5);
}

void getTemp102(){
  byte firstbyte, secondbyte;
  Wire.requestFrom(TMP102_I2C_ADDRESS, 2);
  boolean timeout = false;
  unsigned long t0 = millis();
  do {
    timeout = ((millis() - t0) > TIMEOUT_MILLIS);
  } while (!Wire.available() && !timeout);
  if (!timeout) {
    firstbyte      = (Wire.read()); 
    secondbyte     = (Wire.read());
    //Serial.print(firstbyte, HEX); Serial.print(' '); Serial.println(secondbyte, HEX);
    correctedtemp = (((int)(firstbyte) << 4)|((int)(secondbyte) >> 4)) * 0.0625;
  } else {
    correctedtemp = -273.15;
  }
}

void loop() {
  while (!radio.available());
  radio.read();
  Serial.println((int)radio.data[0]); 
  //Serial.print(F(" - ")); Serial.print((int)radio.data[1]);
  //Serial.print(F(" - ")); Serial.print((int)radio.data[2]); Serial.print(F(" - ")); Serial.print((int)radio.data[3]);
  //Serial.print(F(" - ")); Serial.println((int)radio.data[4]);
  if (radio.data[0] == 1) {
    message = ((unsigned long)radio.data[1] << 24) | ((unsigned long)radio.data[2] << 16) | 
      ((unsigned long)radio.data[3] << 8) | ((unsigned long)radio.data[4]);
    //Serial.println(message, HEX);
    irsend.sendWhynter(message,32);
    if (message == 0x8c002aa5) 
      digitalWrite(13, 1);
    else
      digitalWrite(13, 0);
  } else if (radio.data[0] == 2) {
    radio.txMode(2);
    digitalWrite(13, 1);
    delay(100);
    digitalWrite(13,0);
    getTemp102();
    radio.data[0]=(int)(correctedtemp*10) >> 8;
    radio.data[1]=(int)(correctedtemp*10) & 0x00ff;
    //Serial.print(radio.data[0]); Serial.print(F(" - ")); Serial.println(radio.data[1]);
    radio.write();
    radio.rxMode(5);
  }
}
