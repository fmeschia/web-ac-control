/*
  Keeloq.h - Crypto library
  Written by Frank Kienast in November, 2010
*/
#ifndef Keeloq_h
#define Keeloq_h

#include <stdlib.h>
#include <RF24/RF24.h>

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

class Keeloq
{
  public:
    Keeloq(uint32_t keyHigh, uint32_t keyLow);
    uint32_t encrypt(uint32_t data);
    uint32_t decrypt(uint32_t data);
  private:
    uint32_t _keyHigh;
	uint32_t _keyLow;
};

#endif

