#!Transmit a string through the serial port
all: main.c receiveAndTransmit.c serialCom.h
	gcc -Wall main.c receiveAndTransmit.c -o serialCom

debug: main.c receiveAndTransmit.c serialCom.h
	gcc -g -Wall main.c receiveAndTransmit.c -o serialCom

clean:
	rm -f serialCom
