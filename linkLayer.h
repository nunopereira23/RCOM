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

//
// typedef struct{
// } Frame;
#define Frame_Size 20


typedef struct {
char fd; /* /dev/ttySx File descriptor/*/
int baudRate; /*Velocidade de transmissão*/
unsigned int seqNum;   /*Número de sequência da trama: 0, 1*/
unsigned int timeout; /*Valor do temporizador: 1 s*/
unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
unsigned int frameSize;
unsigned int readBytes;
char frame[Frame_Size]; /*Trama*/
} LinkLayer;

#define BAUDRATE B38400
//#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define TRANSMISSOR 1
#define RECEIVER 0

#define N_TRIES 3

// int serialP; //SerialPort File descriptor

//int transmit(int fd, char message[], unsigned int size);
int receiveframe(LinkLayer* linkLayer, unsigned char controlField);
int readData(LinkLayer* lk);
int bcc2Check(LinkLayer* lk);
int destuffing(LinkLayer* lk);
int stuffing(LinkLayer* lk);



int llwrite(int fd, char * buffer, int length);
int llopen(int port, char transmissor);
void alarmHandler(int sigNum);




//Info Frame
#define INFO 30
#define SEQ_NUM0 0
#define SEQ_NUM1 0x40

//SerialPort Control messages - Supervision
#define FLAG 0x7E
#define ADDRESS 0x03 //A
#define ADDRESS1 0x01 //A
//Control Field - C
#define SET 0x03
#define DISC  0x0B
#define UA  0x07

  //Receive fields MSbit R = N(r)
  #define RR  0x05
    #define RR_0 0x05
    #define RR_1 0x85
  #define REJ 0x01
    #define REJ_0 0x01
    #define REJ_1 0x81
//BCC1  xor(A, C) // Address ^ Control

//stuffing

#define ESC 0x7d
#define ESC_EX 0x5d
#define FLAG_EX 0x5e

#endif
