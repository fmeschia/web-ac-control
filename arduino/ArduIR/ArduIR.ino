/*****

Web A/C Remote Control - the Arduino remote
by Francesco Meschia

This code runs on the ATmega328 that controls the A/C unit.
The main program loop waits for data to be available on either
the serial stream or the nRF24L01+ radio stream.

The commands which can be sent via the radio channel are:
- send IR code to A/C
- read back via radio the temperature from TMP102 TWI sensor

Since sending IR codes means changing the state of a physical
external device, the message must be "authenticated" by using
an encrypted rolling code. The 64-bit cryptographic key and the
sequence seed can be changed by sending, respectively, a
"k <low 32 bits of the key, HEX> <high 32 bits of the key, HEX>"
or a "s <sequence seed, DEC>" command over the serial link.

================================
Copyright 2015 Francesco Meschia

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*****/

#include <IRremote.h>
#include <IRremoteInt.h>
#include <Wire.h>
#include <SPI.h>
#include <RF24.h>
#include <printf.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include "Keeloq.h"

#define DEBUG

#define TMP102_I2C_ADDRESS 72
#define TIMEOUT_MILLIS 100L
#define SLEEP_TIMEOUT 8000L
#define LED_PIN 8

#define LOC_SEQNUM 0x00
#define LOC_KEY_LO 0x04
#define LOC_KEY_HI 0x08

RF24 radio(9, 10);
IRsend irsend;
float correctedtemp;
uint32_t message;
uint32_t encSequenceNumber;
uint32_t decSequenceNumber;
uint32_t sequenceNumber;
Keeloq *k;
int8_t firstbyte, secondbyte;
uint8_t buffer[10];
String s_key_lo, s_key_hi, s_seq;
uint32_t key_lo, key_hi;
unsigned long lastWakeupTime;

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);

  // read 64-bit key as two 32-bit words from EEPROM
  key_lo = ((uint32_t)EEPROM.read(LOC_KEY_LO) << 24) | ((uint32_t)EEPROM.read(LOC_KEY_LO + 1) << 16) |
           ((uint32_t)EEPROM.read(LOC_KEY_LO + 2) << 8) | ((uint32_t)EEPROM.read(LOC_KEY_LO + 3));
  key_hi = ((uint32_t)EEPROM.read(LOC_KEY_HI) << 24) | ((uint32_t)EEPROM.read(LOC_KEY_HI + 1) << 16) |
           ((uint32_t)EEPROM.read(LOC_KEY_HI + 2) << 8) | ((uint32_t)EEPROM.read(LOC_KEY_HI + 3));
#ifdef DEBUG
  Serial.print(F("key_lo=")); Serial.println(key_lo, HEX);
  Serial.print(F("key_hi=")); Serial.println(key_hi, HEX);
#endif

  // read 32-bit sequence number from EEPROM
  sequenceNumber = ((uint32_t)EEPROM.read(LOC_SEQNUM) << 24) | ((uint32_t)EEPROM.read(LOC_SEQNUM + 1) << 16) |
                   ((uint32_t)EEPROM.read(LOC_SEQNUM + 2) << 8) | ((uint32_t)EEPROM.read(LOC_SEQNUM + 3));
#ifdef DEBUG
  Serial.print(F("seq=")); Serial.println(sequenceNumber);
#endif

  // initialize Keeloq generator
  k = new Keeloq(key_lo, key_hi);
  printf_begin();

  // initialize TMP102 temperature sensor
  Wire.begin();
  Wire.beginTransmission(TMP102_I2C_ADDRESS);
  Wire.write(0x01); // write to config registry
  Wire.write(0x01); // two 8-bit words for config registry. Setting bit 0 of byte 1 activates shutdown mode
  Wire.write(0x00);
  Wire.endTransmission();
#ifdef DEBUG
  // print TMP102 diagnostic information if in debug mode
  Wire.beginTransmission(TMP102_I2C_ADDRESS);
  Wire.write(0x01);
  Wire.endTransmission();
  Wire.requestFrom(TMP102_I2C_ADDRESS, 2);
  byte firstbyte      = (Wire.read());
  byte secondbyte     = (Wire.read());
  Serial.print(F("TMP102 CFG ")); Serial.print(firstbyte, HEX); Serial.print(F(" ")); Serial.println(secondbyte, HEX);
#endif
  // initialize nRF2401 radio module
  radio.begin();
  radio.setRetries(15, 15);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);
  radio.startListening();
#ifdef DEBUG
  radio.printDetails();
#endif
  sleepNow();
}

void getTemp102() {
  // start a single one-shot temperateure conversion
  Wire.beginTransmission(TMP102_I2C_ADDRESS);
  Wire.write(0x01); // write to config register
  Wire.write(0x80); // one-shot conversion
  Wire.write(0x00);
  Wire.endTransmission();
  // ask to read the temperature register
  Wire.beginTransmission(TMP102_I2C_ADDRESS);
  Wire.write(0x00); // read temp register
  Wire.endTransmission();
  delay(30); // allow for conversion time
  Wire.requestFrom(TMP102_I2C_ADDRESS, 2);
  boolean timeout = false;
  unsigned long t0 = millis();
  // wait until 2 bytes show up on the TWI bus
  do {
    timeout = ((millis() - t0) > TIMEOUT_MILLIS);
  } while (Wire.available() != 2 && !timeout);
  if (!timeout) {
    // read temperature as 2 8-bit words
    firstbyte      = (Wire.read());
    secondbyte     = (Wire.read());
#ifdef DEBUG
    Serial.print(F("TMP ")); Serial.print(firstbyte, HEX); Serial.print(' '); Serial.println(secondbyte, HEX);
#endif
  } else {
    firstbyte = 0xff; secondbyte = 0xff;
  }
}

ISR (PCINT2_vect) {
}


void WakeHandler() {
}

void sleepNow() {
  Serial.println(F("Sleep..."));
  Serial.flush();

  // pin change interrupt code by Nick Gammon
  noInterrupts ();
  byte old_ADCSRA = ADCSRA;
  // disable ADC
  ADCSRA = 0;
  // pin change interrupt (example for D0)
  PCMSK2 |= bit (PCINT16); // want pin 0
  PCIFR  |= bit (PCIF2);   // clear any outstanding interrupts
  PCICR  |= bit (PCIE2);   // enable pin change interrupts for D0 to D7
  attachInterrupt(0, &WakeHandler, LOW);

  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  power_adc_disable();
  power_spi_disable();
  power_timer0_disable();
  power_timer1_disable();
  power_timer2_disable();
  power_twi_disable();

  UCSR0B &= ~bit (RXEN0);  // disable receiver
  UCSR0B &= ~bit (TXEN0);  // disable transmitter

  sleep_enable();
  interrupts ();
  sleep_cpu ();
  sleep_disable();
  power_all_enable();

  ADCSRA = old_ADCSRA;
  PCICR  &= ~bit (PCIE2);   // disable pin change interrupts for D0 to D7
  detachInterrupt(0);
  UCSR0B |= bit (RXEN0);  // enable receiver
  UCSR0B |= bit (TXEN0);  // enable transmitter

  Serial.println(F("Awake!"));
  lastWakeupTime = millis();
}


void loop() {
  if (Serial.available()) {
    // if serial data, look for commands
    byte c = Serial.read();
    if (c == 'K' || c == 'k') {
      // the 'K" command sets the key: 'k <low 32 bits, hex> <high 32 bits, hex>'
      Serial.readStringUntil(' ');
      s_key_lo = Serial.readStringUntil(' ');
      s_key_hi = Serial.readStringUntil(' ');
      key_lo = strtoul(s_key_lo.c_str(), NULL, 16);
      // give feeback on serial
      Serial.print(F("key_lo_parsed=")); Serial.println(key_lo, HEX);
      key_hi = strtoul(s_key_hi.c_str(), NULL, 16);
      Serial.print(F("key_lo_parsed=")); Serial.println(key_hi, HEX);
      // create new keeloq instance
      if (k) delete k;
      k = new Keeloq(key_lo, key_hi);
      // write key to EEPROM
      EEPROM.write(LOC_KEY_LO, (uint8_t)((key_lo & 0xff000000) >> 24));
      EEPROM.write(LOC_KEY_LO + 1, (uint8_t)((key_lo & 0x00ff0000) >> 16));
      EEPROM.write(LOC_KEY_LO + 2, (uint8_t)((key_lo & 0x0000ff00) >> 8));
      EEPROM.write(LOC_KEY_LO + 3, (uint8_t)((key_lo & 0x000000ff)));
      EEPROM.write(LOC_KEY_HI, (uint8_t)((key_hi & 0xff000000) >> 24));
      EEPROM.write(LOC_KEY_HI + 1, (uint8_t)((key_hi & 0x00ff0000) >> 16));
      EEPROM.write(LOC_KEY_HI + 2, (uint8_t)((key_hi & 0x0000ff00) >> 8));
      EEPROM.write(LOC_KEY_HI + 3, (uint8_t)((key_hi & 0x000000ff)));
    } else if (c == 'S' || c == 's') {
      // the 'S' command sets the Keeloq sequence number: 's <sequence number (decimal)>'
      Serial.readStringUntil(' ');
      s_seq = Serial.readStringUntil(' ');
      sequenceNumber = strtoul(s_seq.c_str(), NULL, 10);
      // give feedback on serial
      Serial.print(F("seq_parsed=")); Serial.println(sequenceNumber);
      // store sequence number in EEPROM
      EEPROM.write(LOC_SEQNUM, (uint8_t)((sequenceNumber & 0xff000000) >> 24));
      EEPROM.write(LOC_SEQNUM + 1, (uint8_t)((sequenceNumber & 0x00ff0000) >> 16));
      EEPROM.write(LOC_SEQNUM + 2, (uint8_t)((sequenceNumber & 0x0000ff00) >> 8));
      EEPROM.write(LOC_SEQNUM + 3, (uint8_t)((sequenceNumber & 0x000000ff)));
    }
    while (Serial.available()) Serial.read();
  }
  if (radio.available()) {
    // radio data come as 9 8-bit words: first word is the command, the remaining words are the operand
    radio.read(buffer, 10);
    Serial.print((int)buffer[0]);
#ifdef DEBUG
    Serial.print(F(" - ")); Serial.print((int)buffer[1], HEX);
    Serial.print(F(" - ")); Serial.print((int)buffer[2], HEX);
    Serial.print(F(" - ")); Serial.print((int)buffer[3], HEX);
    Serial.print(F(" - ")); Serial.print((int)buffer[4], HEX);
#endif
    Serial.println();
    if (buffer[0] == 1) {
      // if command == 1, we are sending IR data. The operand is the 32-bit IR code followed by the 32-bit Keeloq code

      // build IR message
      message = ((uint32_t)buffer[1] << 24) | ((uint32_t)buffer[2] << 16) |
                ((uint32_t)buffer[3] << 8) | ((uint32_t)buffer[4]);
      // read encrypted remote sequence number
      encSequenceNumber = ((uint32_t)buffer[5] << 24) | ((uint32_t)buffer[6] << 16) |
                          ((uint32_t)buffer[7] << 8) | ((uint32_t)buffer[8]);
      // decrypt remote sequence number
      decSequenceNumber = k->decrypt(encSequenceNumber);
      // read local sequence number from EEPROM
      sequenceNumber = ((uint32_t)EEPROM.read(LOC_SEQNUM) << 24) | ((uint32_t)EEPROM.read(LOC_SEQNUM + 1) << 16) |
                       ((uint32_t)EEPROM.read(LOC_SEQNUM + 2) << 8) | ((uint32_t)EEPROM.read(LOC_SEQNUM + 3));
      // command is valid only if the remote sequence number is within the 100 numbers following the local number
      if (decSequenceNumber >= sequenceNumber && decSequenceNumber < sequenceNumber + 100) {
#ifdef DEBUG
        Serial.print(F("Sequence valid: internal=")); Serial.print(sequenceNumber); Serial.print(F(" received=")); Serial.println(decSequenceNumber);
#endif
        sequenceNumber = decSequenceNumber + 1;
        // increment local sequence number and store it in EEPROM
        EEPROM.write(LOC_SEQNUM, (uint8_t)((sequenceNumber & 0xff000000) >> 24));
        EEPROM.write(LOC_SEQNUM + 1, (uint8_t)((sequenceNumber & 0x00ff0000) >> 16));
        EEPROM.write(LOC_SEQNUM + 2, (uint8_t)((sequenceNumber & 0x0000ff00) >> 8));
        EEPROM.write(LOC_SEQNUM + 3, (uint8_t)((sequenceNumber & 0x000000ff)));
        // send A/C IR code
        irsend.sendWhynter(message, 32);
        // toggle visible LED based on the most significant bit of the code (1=on, 0=off)
        if (message & 0x80000000)
          digitalWrite(LED_PIN, 1);
        else
          digitalWrite(LED_PIN, 0);
      } else {
#ifdef DEBUG
        Serial.print(F("Sequence mismatch: internal=")); Serial.print(sequenceNumber); Serial.print(F(" received=")); Serial.println(decSequenceNumber);
#endif
      }
    } else if (buffer[0] == 2) {
      // if command == 1, the remote asked for temperature read

      radio.stopListening();
      // quickly cycle visible LED as a form of acknowledgement
      digitalWrite(LED_PIN, 1);
      getTemp102();
      delay(70);
      digitalWrite(LED_PIN, 0);
      // read temperature
      buffer[0] = firstbyte;
      buffer[1] = secondbyte;
      // pad with b10101010 to maximize the payload "complexity"
      buffer[2] = 0xAA;
      buffer[3] = 0xAA;
      buffer[4] = 0xAA;
#ifdef DEBUG
      Serial.print(F("RTMP ")); Serial.print(buffer[0], HEX); Serial.print(' '); Serial.println(buffer[1], HEX);
#endif
      //send back temperature payload
      radio.write(buffer, 5);
      radio.startListening();
    }
  }
  if (millis() - lastWakeupTime > SLEEP_TIMEOUT) {
    sleepNow();
  }
}
