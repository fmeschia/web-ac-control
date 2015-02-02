#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <RF24/RF24.h>
#include "Keeloq.h"

#undef DEBUG
//#define DEBUG

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
uint8_t buffer[15];
char basepath[500];

int main(int argc, char* argv[])
{
    if (argc != 2) {
        // Tell the user how to run the program
        std::cerr << "Usage: " << argv[0] << " <IR code>" << std::endl;
        /* "Usage messages" are a conventional way of telling the user
         * how to run a program if they enter the command incorrectly.
         */
        return 1;
    }
	ssize_t pathsize = readlink("/proc/self/exe", basepath, 500);
	basepath[pathsize] = 0;
	*strrchr(basepath,'/') = 0; // strip executable name
	std::cout<<"Basepath "<<basepath<<std::endl;
	uint32_t code = strtoul(argv[1],NULL,16);
	unsigned long sequence;
	uint32_t key_lo, key_hi;
	std::fstream myfile;
	myfile.open(strcat(basepath,"/sequence.txt"));
	myfile>>sequence;
	myfile.close();
	*strrchr(basepath,'/') = 0; // strip /sequence.txt
#ifdef DEBUG
	std::cout<<"Sequence "<<std::dec<<sequence<<std::endl;
#endif
	myfile.open(strcat(basepath,"/key.txt"));
	myfile >> std::hex >> key_lo;
	myfile >> std::hex >> key_hi;
	myfile.close();
	*strrchr(basepath,'/') = 0; // strip /key.txt
#ifdef DEBUG
	std::cout<<"Key "<<std::hex<<key_lo<<" "<<key_hi<<std::hex<<std::endl;
#endif
	Keeloq k(key_lo,key_hi);
	RF24 radio(RPI_BPLUS_GPIO_J8_22,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_4MHZ);
	radio.begin();
	radio.setRetries(15,15);
//	radio.setDataRate(RF24_250KBPS);
//	radio.setPayloadSize(10);
    radio.openWritingPipe(pipes[0]);
#ifdef DEBUG
	radio.printDetails();
#endif
	buffer[0]=(unsigned char)1;
	buffer[1]=(unsigned char)((code & 0xff000000) >> 24);
	buffer[2]=(unsigned char)((code & 0x00ff0000) >> 16);
	buffer[3]=(unsigned char)((code & 0x0000ff00) >> 8);
	buffer[4]=(unsigned char)((code & 0x000000ff));
	uint32_t encSequence = k.encrypt(sequence);
	buffer[5]=(unsigned char)((encSequence & 0xff000000) >> 24);
	buffer[6]=(unsigned char)((encSequence & 0x00ff0000) >> 16);
	buffer[7]=(unsigned char)((encSequence & 0x0000ff00) >> 8);
	buffer[8]=(unsigned char)((encSequence & 0x000000ff));
	radio.write(buffer,9);
	//std::cout << (int)radio.data[0] << " - " << (int)radio.data[1] << ":" << (int)radio.data[2] << ":" << (int)radio.data[3] << ":" << (int)radio.data[4] << std::endl;
	//std::cout << "sizeof(unsigned long) is " << sizeof(code) << " bytes " << std::endl;
	radio.powerDown();
	myfile.open (strcat(basepath,"/sequence.txt"), std::fstream::out | std::fstream::trunc);
	myfile<<std::dec<<sequence+1<<std::endl;
	myfile.close();
    return 0;
}