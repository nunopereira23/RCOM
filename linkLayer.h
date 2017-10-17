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
char fd; /*Dispositivo /dev/ttySx, x = 0, 1*/
int baudRate; /*Velocidade de transmissão*/
unsigned int seqNumber;   /*Número de sequência da trama: 0, 1*/
unsigned int timeout; /*Valor do temporizador: 1 s*/
unsigned int numTransmissions; /*Número de tentativas em caso de falha*/
char frame[Frame_Size]; /*Trama*/
} LinkLayer;

LinkLayer linkLayer;

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
int receive(int fd, char message[], unsigned int size);

int llwrite(int fd, char * buffer, int length);
int llopen(int port, char transmissor);
void alarmHandler(int sigNum);




//SerialPort Control messages - Supervision
#define FLAG 0x7E
#define ADDRESS 0x03 //A
//Control Field - C
#define SET 0x03
#define DISC  0x0B
#define UA  0x07

  //Receive fields MSbit R = N(r)
  #define RR  0x05
  #define REJ 0x01
//BCC1  xor(A, C) // Address ^ Control

#endif
