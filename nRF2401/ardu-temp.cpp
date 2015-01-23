#include <iostream>
#include <ctime>
#include <stdlib.h>
#include <cstring>
#include <sys/mman.h>
#include "Nrf2401.h"

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
	radio.power = 3;
	radio.dataRate = 0;
	int ntries = 0;
	int timeout = 0;
	do {
		radio.txMode(5);
		radio.data[0]=(unsigned char)2;
		radio.data[1]=(unsigned char)0;
		radio.data[2]=(unsigned char)0;
		radio.data[3]=(unsigned char)0;
		radio.data[4]=(unsigned char)0;
		radio.write();
		radio.rxMode(2);
		timeout = 0;
		long double time0 = time(0);
		do {
			timeout = ((time(0)-time0) * 1000.0 > 1000.0);
		} while (!radio.available() && !timeout);
	} while (timeout && ntries++ < 5);
	if (!timeout) {
	  	radio.read();
	  	int temp = ((uint16_t)radio.data[0]) << 4 | ((uint16_t)radio.data[1]) >> 4;
	  	std::cout << temp << std::endl;
    } else {
		std::cout << "Timeout" << std::endl;
    }

    return 0;
}
