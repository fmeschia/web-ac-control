/*****

Web A/C Remote Control - the Arduino remote
by Francesco Meschia

This program requests a temperature readout from the remote Arduino unit

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

*/

#include <iostream>
#include <ctime>
#include <cmath>
#include <stdlib.h>
#include <cstring>
#include <sys/mman.h>
#include <RF24/RF24.h>
//#include "Nrf2401.h"

using namespace std;

#undef DEBUG
//#define DEBUG

#define TEMP_OFFSET_C 0.0
#define MAX_TRIES 5
#define TIMEOUT_MS 1000.0

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };
uint8_t buffer[6];

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
	
	RF24 radio(RPI_BPLUS_GPIO_J8_22,RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_4MHZ);
	radio.begin();
	//delay(10);
	radio.setRetries(15,15);
	//radio.setDataRate(RF24_250KBPS);
	//radio.setPayloadSize(5);
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1,pipes[1]);
#ifdef DEBUG
	radio.printDetails();
#endif
	int ntries = 0;
	int done = 0;
	int timeout = 0;
	do {
		//radio.txMode(5);
		buffer[0]=(uint8_t)2;
		buffer[1]=(uint8_t)0;
		buffer[2]=(uint8_t)0;
		buffer[3]=(uint8_t)0;
		buffer[4]=(uint8_t)0;
		radio.write(buffer, 5);
#ifdef DEBUG
		cout << "Wrote" << endl;
#endif
	//	delay(10);
		radio.startListening();
	//	delay(5);
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
				radio.read(buffer, 5);
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
		radio.stopListening();
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
