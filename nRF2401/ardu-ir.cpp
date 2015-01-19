#include <iostream>
#include <stdlib.h>
#include "Nrf2401.h"

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
	Nrf2401 radio;
	radio.remoteAddress = 1;
	radio.localAddress = 2;
	radio.power = 3;
	radio.dataRate = 0;
	radio.txMode(5);
	radio.data[0]=(unsigned char)1;
	radio.data[1]=(unsigned char)((code & 0xff000000) >> 24);
	radio.data[2]=(unsigned char)((code & 0x00ff0000) >> 16);
	radio.data[3]=(unsigned char)((code & 0x0000ff00) >> 8);
	radio.data[4]=(unsigned char)((code & 0x000000ff));
	radio.write();
	//std::cout << (int)radio.data[0] << " - " << (int)radio.data[1] << ":" << (int)radio.data[2] << ":" << (int)radio.data[3] << ":" << (int)radio.data[4] << std::endl;
	//std::cout << "sizeof(unsigned long) is " << sizeof(code) << " bytes " << std::endl;

    return 0;
}