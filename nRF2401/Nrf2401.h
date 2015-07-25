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

#include <bcm2835.h>

#define NRF2401_BUFFER_SIZE 10

#define CE_PIN 5
#define CS_PIN 6
#define DR1_PIN 20
#define CLK_PIN 21
#define DAT_PIN 22

#define _delay_us(D) bcm2835_delayMicroseconds(D)
#define _delay_ms(D) bcm2835_delay(D)

#define SELECT_CHIP   bcm2835_gpio_write(CS_PIN,1)
#define DESELECT_CHIP bcm2835_gpio_write(CS_PIN,0)
#define ENABLE_CHIP   bcm2835_gpio_write(CE_PIN,1)
#define DISABLE_CHIP  bcm2835_gpio_write(CE_PIN,0)
#define CYCLE_CLOCK   bcm2835_gpio_write(CLK_PIN,1), _delay_us(1), bcm2835_gpio_write(CLK_PIN,0)
#define TX_DATA_HI    bcm2835_gpio_write(DAT_PIN,1)
#define TX_DATA_LO    bcm2835_gpio_write(DAT_PIN,0)
#define RX_DATA_HI    bcm2835_gpio_lev(DAT_PIN)
#define DATA_READY    bcm2835_gpio_lev(DR1_PIN)

class Nrf2401
{
  public:
  
  // properties
  
  volatile unsigned char data[NRF2401_BUFFER_SIZE];
  volatile unsigned int remoteAddress;
  volatile unsigned int localAddress;
  volatile unsigned char dataRate;
  volatile unsigned char channel;
  volatile unsigned char power;
  volatile unsigned char mode;
  
  // methods
  
  Nrf2401(void);
  ~Nrf2401();
  void rxMode(unsigned char messageSize=0);
  void txMode(unsigned char messageSize=0);
  void write(unsigned char dataByte);
  void write(unsigned char* dataBuffer=0);
  void read(unsigned char* dataBuffer=0);
  bool available(void);

  // you shouldn't need to use anything below this point..
  
  volatile unsigned char payloadSize;
  volatile unsigned char configuration[15];
  void configure(void);
  void loadConfiguration(bool modeSwitchOnly=false);
  void loadByte(unsigned char byte);
};
