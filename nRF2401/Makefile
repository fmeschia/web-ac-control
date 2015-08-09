CC=g++
RF24LIB=rf24-bcm

all: ardu-ir ardu-temp

ardu-ir: ardu-ir.cpp Keeloq.cpp
	$(CC) -l$(RF24LIB) ardu-ir.cpp Keeloq.cpp -o ardu-ir

ardu-temp: ardu-temp.cpp
	$(CC) -l$(RF24LIB) ardu-temp.cpp -o ardu-temp

clean:
	rm ardu-temp ardu-ir
