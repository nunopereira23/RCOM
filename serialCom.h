#ifndef SERIAL_COM_H
#define SERIAL_COM_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


#define BAUDRATE B38400
//#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define TRANSMISSOR 1
#define RECEIVER 0
#define N_TRIES 3

int serialP; //SerialPort File descriptor

//int transmit(int fd, char message[], unsigned int size);
int receive(int fd, char message[], unsigned int size);
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
