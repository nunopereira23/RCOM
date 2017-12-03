#!Transmit a file through the serial port
serialCom: linkLayer.c linkLayer.h appLayer.c appLayer.h
	gcc -Wall linkLayer.c appLayer.c -lm -o serialCom

debug: linkLayer.c linkLayer.h appLayer.c appLayer.h
	gcc -g -Wall linkLayer.c appLayer.c -lm -o serialCom

clean:
	rm -f serialCom
