#include <iostream>
#include <stdlib.h>
#include <RF24/RF24.h>

#undef DEBUG
//#define DEBUG

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
uint8_t buffer[6];

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
	unsigned long code = strtoul(argv[1],NULL,16);
	RF24 radio(RPI_BPLUS_GPIO_J8_22,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_4MHZ);
	radio.begin();
	radio.setRetries(15,15);
//	radio.setDataRate(RF24_250KBPS);
	radio.setPayloadSize(5);
    radio.openWritingPipe(pipes[0]);
#ifdef DEBUG
	radio.printDetails();
#endif
	buffer[0]=(unsigned char)1;
	buffer[1]=(unsigned char)((code & 0xff000000) >> 24);
	buffer[2]=(unsigned char)((code & 0x00ff0000) >> 16);
	buffer[3]=(unsigned char)((code & 0x0000ff00) >> 8);
	buffer[4]=(unsigned char)((code & 0x000000ff));
	radio.write(buffer, 5);
	//std::cout << (int)radio.data[0] << " - " << (int)radio.data[1] << ":" << (int)radio.data[2] << ":" << (int)radio.data[3] << ":" << (int)radio.data[4] << std::endl;
	//std::cout << "sizeof(unsigned long) is " << sizeof(code) << " bytes " << std::endl;
	radio.powerDown();
    return 0;
}