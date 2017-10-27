#!Transmit an image .gif through the serial port
serialCom: main.c linkLayer.c linkLayer.h appLayer.c appLayer.h
	gcc -Wall main.c linkLayer.c appLayer.c -o serialCom

debug: main.c linkLayer.c linkLayer.h
	gcc -g -Wall main.c linkLayer.c appLayer.c -o serialCom

clean:
	rm -f serialCom
