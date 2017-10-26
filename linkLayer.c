#include "linkLayer.h"
#include "stateMachines.h"

unsigned int retryCount, state;

/**
* @ llopen() SigAlm handler, it increments nTries
* and changes state to SET_SEND
*/

void alarmHandler(int sigNum){
  retryCount++;
  printf("Alarm triggered\n");
  state = SET_SEND;
}


/**
* @ Establishes a serial connection between two machines
* @ parm port -
* @ parm flag - boolean flag, 0 for emmisor and 1 stands for receiver
* @ return return the serial port's fd or a negative number if an error occurs
*/
int llopen(int port, char flag){
  int serialPort;
  unsigned char message[5];
  unsigned char byte;

  if(flag != TRANSMISSOR && flag != RECEIVER){
    perror("llopen()::Couldn't open serialPort fd\n");
    return -1;
  }

  char path[] = "/dev/ttyS", portString[2];

  portString[0] =  port + '0';
  portString[1] = '\0';

  strcat(path, portString);


  //printf("path %s\n", path);
  if((serialPort = open(path, O_RDWR | O_NOCTTY )) < 0){
    printf("llopen()::Couldn't open serialPort %d\n", port);
    return -1;
  }


  struct termios oldtio, newtio;

  if ( tcgetattr(serialPort,&oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

  tcflush(serialPort, TCIOFLUSH);

  if ( tcsetattr(serialPort,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }
//     LinkLayer lk; //TO TEST receiveframe()
//     lk.fd = serialPort;
//     receiveframe(&lk, INFO);
//     int i;
//     for(i = 0; i < 12; i++)
//     printf("%02x |", lk.frame[i]);
//
// printf("\n");
// return 0;

  if(flag == TRANSMISSOR){
    int connected = 0;
    retryCount = 0;

    state = SET_SEND;

    while(!connected &&  retryCount != N_TRIES){
      if(state != SET_SEND && state != END){
        read(serialPort, &byte, 1);
        printf("UA %x\n", byte);
      }
      switch (state) {
        case  SET_SEND:
          message[0] = FLAG;
          message[1] = ADDRESS;
          message[2] = SET;
          message[3] = SET ^ ADDRESS;
          message[4] = FLAG;

          write(serialPort, message, 5); //Send set message
           if(alarm(TIMEOUT) != 0){
             printf("Alarm already scheduled in seconds\n");
           }
          //WAIT FOR UA
          state = START;
          //printf("State\n");
          break;
        case  START://UA State Machine
            //printf("START\n");
            if(byte == FLAG)
              state = FLAG_RCV;
            break;

          case FLAG_RCV:
            if(byte == ADDRESS){
              state = A_RCV;
            }
            else if(byte == FLAG)
              state = FLAG_RCV;
            else
              state = START;
          break;

          case A_RCV:
          //printreceiveframef("Reached Address Received\n");
            if(byte == UA)
              state = UA_RCV;
            else if(byte == FLAG)
              state = FLAG_RCV;
            else
              state = START;
          break;

          case UA_RCV:
          //printf("Reached UA (C) Received\n");
            if(byte == (ADDRESS ^ UA))
              state = BCC_OK;
            else if(byte == FLAG)
              state = FLAG_RCV;
            else
              state = START;
          break;

          case BCC_OK:
            if(byte == FLAG){
              state = END;
              //printf("Reached BCC_Ok, state %d\n", state);
            }
            else
              state = START;
          break;

          case END:
            printf("Right Before Disabling Alarm\n");
            alarm(0); //Disables the alarm
            connected = TRUE;
            printf("Received an UA\n");
          break;
        }
      }
    if(retryCount == N_TRIES){
      printf("llopen failed due to the number o retries reaching it's limit\n");
      return -1;
    }

  }

  else{
      unsigned int conEstab = 0;
      state = START;
      while(!conEstab){
        // printf("Receiver State %d\n", state); DEBUG
        if(state != END){
          read(serialPort, &byte, 1);
          printf("%x\n", byte);
        }
        switch (state) { //Check if SET Message is received
          case START:
            if(byte == FLAG)
              state = FLAG_RCV;
          break;

          case FLAG_RCV:
            if(byte == ADDRESS)
              state = A_RCV;
            else if(byte == FLAG)
              state = FLAG_RCV;
            else
              state = START;
          break;

          case A_RCV:
            if(byte == SET)
              state = SET_RCV;
            else if(byte == FLAG)
              state = FLAG_RCV;
            else
              state = START;
          break;

          case SET_RCV:
            if(byte == (ADDRESS ^ SET))
              state = BCC_OK;
            else if(byte == FLAG)
              state = FLAG_RCV;
            else
              state = START;
          break;

          case BCC_OK:
            if(byte == FLAG)
              state = END;
            else
              state = START;
          break;

          case END:
            conEstab = TRUE;
          break;
        }
      }

      //After a successful SET message was received, send a UA
      printf("Starting to send UA\n");
      message[0] = FLAG;
      message[1] = ADDRESS;
      message[2] = UA;
      message[3] = UA ^ ADDRESS;
      message[4] = FLAG;
      if(write(serialPort, message, 5) == 0)
        printf("Failed to transmit an UA\n");
  }
	printf("Connection established\n");
  return serialPort;
}

int llwrite(int fd, unsigned char * buffer, int length){
  retryCount = 0;

  unsigned char bcc2 = calcBcc2(buffer, length);
  linkLaye.frame[0] = FLAG;
  linkLayer.frame[1] = ADDRESS;
  linkLayer.frame[2] = 0;
  linkLayer.frame[3] = 0;
  memmove(linkLayer.frame+4, buffer, length);
  linkLayer.frame[3+length] = bcc2;
  stuffing(linkLayer.frame+ 4, &(length + 1));
  linkLayer.frameSize = length + 6;
  linkLayer.frame[linkLayer.frameSize-1] = FLAG;

  unsigned char sent = 0, bytesWritten = 0;
  while(sent && retryCount < N_TRIES){
    switch (state) {
      case START:
        bytesWritten = write(fd, linkLayer.frame, linkLayer.frameSize);
        alarm(TIMEOUT);
        state = RECEIVE;
      case RECEIVE:
        receiveframe(linkLayer);
        if()
      case END:
        alarm(0);
        sent = 1;
      break;
    }
  }
  return bytesWritten;
}

int llread(int fd, char * buffer){
  buffer = linkLayer.frame;
  unsigned char message[] = {FLAG, ADDRESS, 0, 0, FLAG};

    char response = receiveframe(&linkLayer, INFO);
    message[2] = response;
    message[3] = response ^ ADDRESS;
    write(fd, message, 5);

  return linkLayer.readBytes;
}


/**
* Generic frame receiver, it can handle Info Frames as well as Supervision Frame_Size
* @param framelkLayer
* @param controlField expected  frame's contcontrolFieldrol field
* @return 0 if received without errors and  RR(Num) or REJ(Num) if there's a proble with data within the frame
*/
int receiveframe(LinkLayer* lkLayer, unsigned char controlField){ //ADDRESS 0x03 or 0x01
	int stop = 0;
  unsigned int newSeqNum;

	int i = 0;
	state = START;

	while(!stop){
		switch (state) {
			case START:
				read(lkLayer->fd, lkLayer->frame + i, 1);
				if(lkLayer->frame[i] == FLAG){
					state = FLAG_RCV;
					i++;
				}
				break;
			case FLAG_RCV:
				read(lkLayer->fd, lkLayer->frame + i, 1);
				if(lkLayer->frame[i] == ADDRESS){
					state = A_RCV;
					i++;
				}
				else if(lkLayer->frame[i] != FLAG){//Other unexpected info
					state = START;
					i = 0;
				}
				break;

			case A_RCV:
				read(lkLayer->fd, lkLayer->frame + i, 1);

        if(lkLayer->frame[i] == controlField){
					state = C_RCV;
					i++;
				}
				break;

				if(controlField == INFO){
            lkLayer->readBytes = 0;
            newSeqNum = lkLayer->frame[i] >> 6;
            i++;
            read(lkLayer->fd, lkLayer->frame + i, 1);
            if((lkLayer->frame[i-2] ^ lkLayer->frame[i-1]) == lkLayer->frame[i]){//BCC1 Check <=> A ^ C (seqNum) = BCC1
              if(lkLayer->seqNum == newSeqNum)
                  return RR((newSeqNum+1)%2);


              if(readData(lkLayer) == 0){
                lkLayer->seqNum = newSeqNum;
                return RR(newSeqNum);
              }
              else
                return REJ(lkLayer->seqNum); //Requesting  REJ_0
            }
            i--;
            break;
          }

			case C_RCV:
				read(lkLayer->fd, lkLayer->frame + i, 1);
				if(lkLayer->frame[i] == (ADDRESS ^ lkLayer->frame[i-1])){
					state = BCC1_OK;
					i++;
				}
				break;
			case BCC1_OK:
				read(lkLayer->fd, lkLayer->frame + i, 1);
				if(lkLayer->frame[i] == FLAG){
					state = END;
				}
				break;
			case END:
				stop = 1;
				break;
		}
	}
  return 0;
}

/**
* Reads the data in a Information frame
* @param lk - LinkLayer's info struct
* @return 0 if there's no error, 1 for destuffing error and 2 for Bcc2 check error
* Se se tratar dum duplicado, deve fazer-se confirmação com RR*/
int readData(LinkLayer* lk){
  char stop = 0;
  unsigned int i = 0;
  lk->readBytes = 0;
  while(!stop){
    if(read(lk->fd, &lk->frame[i], 1) == 1){
      lk->readBytes ++;
      i++;
    }

    if(lk->frame[i-1] == FLAG){
      stop = 1;
      lk->readBytes --; //Subtracted non data - wrong increment
    }
  }

    lk->frameSize = lk->readBytes; //FrameSize -> data + bbc2

  if(destuffing(lk) != 0){
    printf("Destuffing Error\n");
    lk->readBytes = 0;
    return 1;
  }

  if(bcc2Check(lk)){
    printf("Failed BCC2 check\n");
    lk->readBytes = 0;
    return 2;
  }

  return 0;
}

int bcc2Check(LinkLayer* lk){
  int i;
  char xorResult = lk->frame[0]; //D0
  printf("BCC2Check %d\n", lk->frameSize);
  for (i = 1; i < lk->frameSize-1; i++) {
    xorResult ^= lk->frame[i];
  }
                  /*BCC2*/
  if(xorResult != lk->frame[lk->frameSize-1])
    return 1;

  lk->frameSize --;//BCC2 Ignored (Deleted)
  return 0; //BCC2 field is correct
}


int stuffing(char* buff, unsigned int* size){

  unsigned int i;

  for(i=0; i< size; i++){
    if(buff[i] == FLAG){
      memmove(&buff[i+2], &buff[i+1], size - (i+1));
      buff[i] = ESC;
      buff[i+1] = FLAG_EX;
      size++;
    }

    if(buff[i] == ESC){
      memmove(&buff[i+2], &buff[i+1], size - (i+1));
      buff[i+1] = ESC_EX;
      size++;
    }
  }
  return 0;
}

int destuffing(LinkLayer* lk){

  unsigned int i, deletedBytes = 0;

  for(i=0; i< lk->frameSize; i++){
    if(lk->frame[i] == ESC){
      if(lk->frame[i+1]==FLAG_EX){
        lk->frame[i+1]= FLAG;
      }
      else if(lk->frame[i+1]==ESC_EX){
        lk->frame[i+1]= ESC;
      }
      else{
        printf("Unstuffing error\n");
        return -1;
      }
      memmove(&lk->frame[i], &lk->frame[i+1], lk->frameSize - (i+1));
      lk->frameSize--;
      deletedBytes++;
    }
  }
  return 0;
}
