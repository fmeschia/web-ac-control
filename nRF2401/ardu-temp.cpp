#include <iostream>
#include <ctime>
#include <cmath>
#include <stdlib.h>
#include <cstring>
#include <sys/mman.h>
#include <bcm2835.h>
#include "Nrf2401.h"

using namespace std;

#undef DEBUG
//#define DEBUG

#define TEMP_OFFSET_C -1.65
#define MAX_TRIES 5
#define TIMEOUT_MS 1000.0

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
uint8_t buffer[10];

int main(int argc, char* argv[])
{
    if (argc != 1) {
        // Tell the user how to run the program
        std::cerr << "Usage: " << argv[0] <<  std::endl;
        /* "Usage messages" are a conventional way of telling the user
         * how to run a program if they enter the command incorrectly.
         */
        return 1;
    }
	
	struct sched_param sp;
	memset(&sp, 0, sizeof(sp));
    sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
	sched_setscheduler(0, SCHED_FIFO, &sp);
	mlockall(MCL_CURRENT | MCL_FUTURE);

    Nrf2401 radio;
    radio.remoteAddress = 1;
	radio.localAddress = 2;
	
	int ntries = 0;
	int done = 0;
	int timeout = 0;
	do {
		radio.txMode(10);
		buffer[0]=(uint8_t)2;
		buffer[1]=(uint8_t)0;
		buffer[2]=(uint8_t)0;
		buffer[3]=(uint8_t)0;
		buffer[4]=(uint8_t)0;
		delay(1);
		radio.write(buffer);
#ifdef DEBUG
		cout << "Wrote" << endl;
#endif
		radio.rxMode(10);
		delay(1);
#ifdef DEBUG
		cout << "Listening" << endl;
#endif
		timeout = 0;
		long double time0 = time(0);
		//while (!radio.available()) ;
		do {
			if (radio.available()) {
#ifdef DEBUG
				cout << "Available" << endl;
#endif
				radio.read(buffer);
#ifdef DEBUG
				cout << "Read" << endl;
				std::cout << (int)buffer[0] << " - " << (int)buffer[1] << " - " << (int)buffer[2] << " - " << (int)buffer[3] << " - " << (int)buffer[4] << std::endl;
#endif
				done = 1;
			}
			/*
			if (radio.failureDetected) {
#ifdef DEBUG
				cout << "Failure detected" << endl;
#endif
				radio.begin();
				radio.failureDetected = 0;
			    radio.openWritingPipe(pipes[0]);
			    radio.openReadingPipe(1,pipes[1]);
			}
			*/
			timeout = ((time(0)-time0) * 1000.0 > TIMEOUT_MS);
		} while (!done && !timeout);
	} while (!done && ntries++ < MAX_TRIES);
	if (done) {
		float temp = ((((uint16_t)buffer[0]) << 4 | ((uint16_t)buffer[1]) >> 4)*0.0625)+TEMP_OFFSET_C;
		std::cout << temp << std::endl;
    } else {
		std::cout << "Timeout" << std::endl;
    }
    radio.powerDown();
    return 0;
}
