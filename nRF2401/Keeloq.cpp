/*
  Keeloq.cpp - Keeloq encryption/decryption
  Written by Frank Kienast in November, 2010
*/


#include "Keeloq.h"
#include <stdlib.h>
#include <RF24/RF24.h>

#define KeeLoq_NLF              0x3A5C742EUL

Keeloq::Keeloq(uint32_t keyHigh, uint32_t keyLow)
{
  _keyHigh = keyHigh;
  _keyLow = keyLow;
}

uint32_t Keeloq::encrypt(uint32_t data)
{
  uint32_t x = data;
  uint32_t r;
  int keyBitNo, index;
  uint32_t keyBitVal,bitVal;

  for (r = 0; r < 528; r++)
  {
    keyBitNo = r & 63;
    if(keyBitNo < 32)
      keyBitVal = bitRead(_keyLow,keyBitNo);
    else
      keyBitVal = bitRead(_keyHigh, keyBitNo - 32);
    index = 1 * bitRead(x,1) + 2 * bitRead(x,9) + 4 * bitRead(x,20) + 8 * bitRead(x,26) + 16 * bitRead(x,31);
    bitVal = bitRead(x,0) ^ bitRead(x, 16) ^ bitRead(KeeLoq_NLF,index) ^ keyBitVal;
    x = (x>>1) ^ bitVal<<31;
  }
  return x;
}

uint32_t Keeloq::decrypt(uint32_t data)
{
  uint32_t x = data;
  uint32_t r;
  int keyBitNo, index;
  uint32_t keyBitVal,bitVal;

  for (r = 0; r < 528; r++)
  {
    keyBitNo = (15-r) & 63;
    if(keyBitNo < 32)
      keyBitVal = bitRead(_keyLow,keyBitNo);
    else
      keyBitVal = bitRead(_keyHigh, keyBitNo - 32);
    index = 1 * bitRead(x,0) + 2 * bitRead(x,8) + 4 * bitRead(x,19) + 8 * bitRead(x,25) + 16 * bitRead(x,30);
    bitVal = bitRead(x,31) ^ bitRead(x, 15) ^ bitRead(KeeLoq_NLF,index) ^ keyBitVal;
    x = (x<<1) ^ bitVal;
  }
  return x;
 }
 

