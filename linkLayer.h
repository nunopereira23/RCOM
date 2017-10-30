#ifndef LINK_LAYER_H
#define LINK_LAYER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define FRAME_SIZE 1024
#define FRAME_I_DATA 1018


typedef struct {
char prog;
int fd; /* /dev/ttySx File descriptor/*/
unsigned int seqNum;
unsigned int frameSize;
unsigned int readBytes;
unsigned char frame[FRAME_SIZE]; /*Trama*/
} LinkLayer;

LinkLayer linkLayer;//Global variable


#define BAUDRATE B38400
//#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define TRANSMISSOR 1
#define RECEIVER 0

#define TIMEOUT 3
#define N_TRIES 3

int receiveFrame(LinkLayer* linkLayer);
int readData(LinkLayer* lk);
int bcc2Calc(unsigned char* buffer, int length);
int bcc2Check(LinkLayer* lk);
int destuffing(LinkLayer* lk);
int stuffing(unsigned char* buff, unsigned int* size);

int llopen(int port, char transmissor);
int llread(int fd, unsigned char * buffer);
int llwrite(int fd, unsigned char * buffer, unsigned int length);
int llclose(int fd);
void alarmHandler(int sigNum);


#define C_IDX 2

//Returns
#define callDisk -1

//Info Frame
#define INFO 30

#define SEQ_NUM(NUM) (NUM << 6)
#define SEQ_NUM0 0
#define SEQ_NUM1 0x40

//SerialPort Control messages - Supervision
#define FLAG 0x7E
#define ADDRESS 0x03
#define ADDRESS1 0x01
//Control Field - C
#define SET 0x03
#define DISC  0x0B
#define UA  0x07

/*Receive fields MSbit R = N(r)*/
  // RR_0 0x05
  // RR_1 0x85
  #define RR(Num) (0x05 | Num << 7)

  //REJ_0 0x01
  //REJ_1 0x81
  #define REJ(Num) (0x1 | Num << 7)

//Stuffing
#define ESC 0x7d
#define ESC_EX 0x5d
#define FLAG_EX 0x5e

#endif
