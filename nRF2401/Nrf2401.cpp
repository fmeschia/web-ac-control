/*
 *  Nrf2401.h
 *  A simplistic interface for using Sparkfun's Nrf2401A breakout boards with Arduino
 *  Original code for http://labs.ideo.com by Jesse Tane March 2009
 *
 *  License:
 *  --------
 *  This is free software. You can redistribute it and/or modify it under
 *  the terms of Creative Commons Attribution 3.0 United States License. 
 *  To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/us/ 
 *  or send a letter to Creative Commons, 171 Second Street, Suite 300, San Francisco, California, 94105, USA.
 *
 *  Notes:
 *  ------
 *  For documentation on how to use this library, please visit http://www.arduino.cc/playground/Main/InterfacingWithHardware
 *  Pin connections should be as follows for Arduino:
 *
 *  DR1 = 2  (digital pin 2)
 *  CE  = 3
 *  CS  = 4
 *  CLK = 5
 *  DAT = 6
 *
 */

#include <stdio.h>
#include "Nrf2401.h"

Nrf2401::Nrf2401(void)
{
  bcm2835_init();
  bcm2835_gpio_fsel(DR1_PIN, BCM2835_GPIO_FSEL_INPT);
  bcm2835_gpio_fsel(CE_PIN, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(CS_PIN, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(CLK_PIN, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(DAT_PIN, BCM2835_GPIO_FSEL_OUTP);
  
  remoteAddress = 0;
  localAddress = 0;
  payloadSize = 0;
  dataRate = 1;
  channel = 111;
  power = 3;
  mode = 0;
  
  configuration[7] = 234; // 3 most significant bytes of the address
  configuration[8] = 223;
  configuration[9] = 212;
  
  DISABLE_CHIP;
  DESELECT_CHIP;
}

void Nrf2401::rxMode(unsigned char messageSize)
{
  bcm2835_gpio_fsel(DAT_PIN, BCM2835_GPIO_FSEL_OUTP);
  mode = 1;
  if(messageSize) payloadSize = messageSize, configure();
  else configuration[14] |= 1, loadConfiguration(true);
  bcm2835_gpio_fsel(DAT_PIN, BCM2835_GPIO_FSEL_INPT);
  ENABLE_CHIP;
  _delay_us(250);
}

void Nrf2401::txMode(unsigned char messageSize)
{
  bcm2835_gpio_fsel(DAT_PIN, BCM2835_GPIO_FSEL_OUTP);
  mode = 0;
  if(messageSize) payloadSize = messageSize, configure();
  else configuration[14] &= ~1, loadConfiguration(true);
  _delay_us(250);
}

void Nrf2401::write(unsigned char* dataBuffer)
{
  if(!dataBuffer) dataBuffer = (unsigned char*) data;
  ENABLE_CHIP;
  _delay_us(5);
  loadByte(configuration[7]);
  loadByte(configuration[8]);
  loadByte(configuration[9]);
  loadByte(remoteAddress >> 8);
  loadByte(remoteAddress);
  for(int i=0; i<payloadSize; i++) loadByte(dataBuffer[i]);
  DISABLE_CHIP;
  _delay_us(500);
}

void Nrf2401::write(unsigned char dataByte)
{
  data[0] = dataByte;
  write();
}

void Nrf2401::read(unsigned char* dataBuffer)
{
  if(!dataBuffer) dataBuffer = (unsigned char*) data;
  DISABLE_CHIP;
  _delay_ms(2);
  for(int i=0; i<payloadSize; i++)
  {
    dataBuffer[i] = 0;
    for(int n=7; n>-1; n--)
    {
      if(RX_DATA_HI) dataBuffer[i] |= (1 << n);
      _delay_us(1);
      CYCLE_CLOCK;
    }
  }
  ENABLE_CHIP;
  _delay_us(1);
}

bool Nrf2401::available(void)
{
  return DATA_READY;
}

//// you shouldn't need to call directly any of the methods below this point...

void Nrf2401::configure(void)
{
  configuration[1] = payloadSize << 3; // payloadSize is bytes; ASL 3 times = x8 (=bits)
  configuration[10] = localAddress >> 8;
  configuration[11] = localAddress;
  configuration[12] = 163; // 163 = CRC enabled, 16-bit CRC, 40-bit address
  configuration[13] = power | (dataRate << 5) | 76; // 76 = 16 MHz Xtal, ShockBurst mdoe
  configuration[14] = mode | (channel << 1);
  loadConfiguration();
}

void Nrf2401::loadConfiguration(bool modeSwitchOnly)
{
  DISABLE_CHIP;
  SELECT_CHIP;
  _delay_us(5);
  if(modeSwitchOnly) loadByte(configuration[14]);
  else for(int i=0; i<15; i++) loadByte(configuration[i]);
  DESELECT_CHIP;
}

void Nrf2401::loadByte(unsigned char byte)
{
  for(int i=7; i>-1; i--)
  {
    if((byte & (1 << i)) == 0) TX_DATA_LO;
    else                       TX_DATA_HI;
    _delay_us(1);
    CYCLE_CLOCK;
  }
}

/*
main() {
	Nrf2401 radio;
	radio.remoteAddress = 1;
	radio.localAddress = 2;
	radio.power = 3;
	radio.dataRate = 0;
	radio.txMode(1);
	//_delay_ms(5);
	//radio.rxMode();
	//radio.read();
	//while (!radio.available());
	//radio.read();
	_delay_ms(500);
	radio.write((unsigned char)1);
	_delay_ms(500);
	radio.write((unsigned char)0);
	_delay_ms(500);
        radio.write((unsigned char)2);
	_delay_ms(1);
        //_delay_ms(2000);	
	radio.rxMode(1);
	while (!radio.available());
	radio.read();
	printf("%d\n",radio.data[0]);
}
*/