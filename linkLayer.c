#include "linkLayer.h"
#include "stateMachines.h"

unsigned int retryCount, state, stateRcv;
static struct termios oldtio;

/**
* @ SigAlm handler, it increments nTries
* and changes state to SET_SEND
*/

void alarmHandler(int sigNum){
  retryCount++;
  printf("Alarm triggered, retryCount =  %d\n", retryCount);
  stateRcv = END;
  state = START;
}


/**
* @ Establishes a serial connection between two machines
* @ parm port
* @ parm flag - boolean flag, 0 for emmisor and 1 stands for receiver
* @ return return the serial port's fd or a negative number if an error occurs
*/
int llopen(int port, char flag){
  unsigned char message[5];

  if(flag != TRANSMISSOR && flag != RECEIVER){
    perror("llopen()::Couldn't open serialPort fd\n");
    return -1;
  }

  char path[] = "/dev/ttyS", portString[2];

  portString[0] =  port + '0';
  portString[1] = '\0';

  strcat(path, portString);

  if((linkLayer.fd = open(path, O_RDWR | O_NOCTTY )) < 0){
    printf("llopen()::Couldn't open serialPort %d\n", port);
    return -1;
  }


  struct termios newtio;

  if ( tcgetattr(linkLayer.fd, &oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 0;
  newtio.c_cc[VMIN]     = 1;

  tcflush(linkLayer.fd, TCIOFLUSH);

  if ( tcsetattr(linkLayer.fd, TCSANOW, &newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  if(flag == TRANSMISSOR){
    int connected = 0;
    retryCount = 0;

    state = START;

    while(!connected &&  retryCount != N_TRIES){
      switch (state) {
        case  START:
          message[0] = FLAG;
          message[1] = ADDRESS;
          message[2] = SET;
          message[3] = SET ^ ADDRESS;
          message[4] = FLAG;

          write(linkLayer.fd, message, 5); //Send set message
           if(alarm(TIMEOUT) != 0){
             printf("Alarm already scheduled in seconds\n");
           }
          state = RECEIVE;
          break;
        case  RECEIVE://UA State Machine
          receiveFrame(&linkLayer);
          if(linkLayer.frame[C_IDX] == UA)
            state = END;
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
        switch (state) { //Check if SET Message is received
          case START:
            receiveFrame(&linkLayer);
            if(linkLayer.frame[C_IDX] == SET)
              state = END;
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
      if(write(linkLayer.fd, message, 5) == 0)
        printf("Failed to transmit an UA\n");
  }
	printf("Connection established\n");
  return linkLayer.fd;
}

int llwrite(int fd, unsigned char* buffer, unsigned int length){
  retryCount = 0;
  unsigned int lenghtStuffng = length +1;
  unsigned char bcc2 = bcc2Calc(buffer, length);
  printf("Bcc2 %x\n", bcc2);
  linkLayer.frame[0] = FLAG;
  linkLayer.frame[1] = ADDRESS;
  linkLayer.frame[2] = SEQ_NUM(linkLayer.seqNum);
  linkLayer.frame[3] = SEQ_NUM(linkLayer.seqNum) ^ ADDRESS;//BCC1
  memmove(linkLayer.frame+4, buffer, length);
  linkLayer.frame[4+length] =  bcc2; //bcc2;

  // int i;
  // for (i = 0; i < 5+ length; i++) {
  //   printf("%x\n", linkLayer.frame[i]);
  // }
  // printf("\n\n\n");

  stuffing(linkLayer.frame + 4, &(lenghtStuffng));
  linkLayer.frameSize = lenghtStuffng + 5;
  linkLayer.frame[linkLayer.frameSize-1] = FLAG;

  // printf("After Stuffing\n");
  // for (i = 0; i < linkLayer.frameSize; i++) {
  //   printf("%x\n", linkLayer.frame[i]);
  // }
  // printf("\n\n\n");

  unsigned char frameCpy[linkLayer.frameSize];
  unsigned int frameISize = linkLayer.frameSize;
  memmove(frameCpy, linkLayer.frame, linkLayer.frameSize);

  state=START;

  unsigned char sent = 0;
  unsigned int bytesWritten = 0;
  while(!sent && retryCount < N_TRIES){
    switch (state) {
      case START:
        linkLayer.frameSize = frameISize;
        memmove(linkLayer.frame, frameCpy, linkLayer.frameSize);
        bytesWritten = write(fd, linkLayer.frame, linkLayer.frameSize);
        printf("BytesWritten %d\n", bytesWritten);
        alarm(TIMEOUT);
        state = RECEIVE;
      break;
      case RECEIVE:
        if(receiveFrame(&linkLayer)){
          printf("llwrite: expected control frame but received Info instead\n");
          printf("CONTROL FIELD %x\n", linkLayer.frame[C_IDX]);
        }
        else if(linkLayer.frame[C_IDX] == RR(0) || linkLayer.frame[C_IDX] == RR(1)){
          state = END;
          printf("llwrite RR\n");
        }
        else if(linkLayer.frame[C_IDX] == REJ(0) || linkLayer.frame[C_IDX] == REJ(1)){
          printf("llwrite REJ\n");
          state = START;
        }
      break;

      case END:
        alarm(0);
        linkLayer.seqNum = (linkLayer.seqNum + 1) % 2;
        sent = 1;
      break;
    }
  }
  if(retryCount == N_TRIES){
    printf("llwrite::Exceeded number of tries\n");
    exit(1);
  }

  return bytesWritten - (lenghtStuffng-length) - 6; //FRAME_HEADER
}

int llread(int fd, unsigned char * buffer){
  unsigned char message[] = {FLAG, ADDRESS, 0, 0, FLAG};
  int i;
  for(i = 0; i < linkLayer.frameSize; i++)
    printf("%x\n", linkLayer.frame[i]);

  unsigned char response = receiveFrame(&linkLayer);
  printf("llread will send %x\n", response);
  message[2] = response;
  message[3] = response ^ ADDRESS;
  write(fd, message, 5);

  memcpy(buffer, linkLayer.frame, linkLayer.readBytes);
  return linkLayer.readBytes;
}

int possibleControlField(unsigned char controlField){
    if(controlField == SET || controlField ==  UA || controlField ==  DISC ||
    controlField ==  RR(0) || controlField ==  RR(1) ||
    controlField ==  REJ(0) ||
    controlField ==  REJ(1))
      return 1;
    return 0;
}

/**
* Generic frame receiver, it can handle Info Frames as well as Supervision Frame_Size
* @param framelkLayer
* @param controlField expected  frame's contcontrolFieldrol field
* @return 0 if received without errors and  RR(Num) or REJ(Num) if there's a proble with data within the frame
*/
int receiveFrame(LinkLayer* lkLayer){ //ADDRESS 0x03 or 0x01
	int stop = 0;
  unsigned int newSeqNum;

	int i = 0;
	stateRcv = START;

	while(!stop){
		switch (stateRcv) {
			case START:
        memset(lkLayer->frame, 0, lkLayer->frameSize);
				read(lkLayer->fd, lkLayer->frame + i, 1);
				if(lkLayer->frame[i] == FLAG){
					stateRcv = FLAG_RCV;
					i++;
				}
				break;
			case FLAG_RCV:
        read(lkLayer->fd, lkLayer->frame + i, 1);
				if(lkLayer->frame[i] == ADDRESS){
					stateRcv = A_RCV;
					i++;
				}
				else if(lkLayer->frame[i] != FLAG){//Other unexpected info
					stateRcv = START;
					i = 0;
				}
				break;

			case A_RCV:
				read(lkLayer->fd, lkLayer->frame + i, 1);

        if(possibleControlField(lkLayer->frame[i])){
					stateRcv = C_RCV;
					i++;
				}

				else if(lkLayer->frame[i] == SEQ_NUM0 || lkLayer->frame[i] == SEQ_NUM1){
            lkLayer->readBytes = 0;
            newSeqNum = lkLayer->frame[i] >> 6;
            i++;
            read(lkLayer->fd, lkLayer->frame + i, 1);
            if((lkLayer->frame[i-2] ^ lkLayer->frame[i-1]) == lkLayer->frame[i]){//BCC1 Check <=> A ^ C (seqNum) = BCC1
              if(lkLayer->seqNum == newSeqNum){
                  printf("Duplicated frame, %d\n", newSeqNum);
                  return RR((newSeqNum+1)%2);
              }


              if(readData(lkLayer) == 0){
                lkLayer->seqNum = (lkLayer->seqNum+1)%2;
                return RR(newSeqNum);
              }
              else
                return REJ(lkLayer->seqNum); //Requesting  REJ_0
            }
            i--;
            break;
          }
      break;

			case C_RCV:
				read(lkLayer->fd, lkLayer->frame + i, 1);
				if(lkLayer->frame[i] == (ADDRESS ^ lkLayer->frame[i-1])){
					stateRcv = BCC1_OK;
					i++;
				}
				break;
			case BCC1_OK:
				read(lkLayer->fd, lkLayer->frame + i, 1);
				if(lkLayer->frame[i] == FLAG){
					stateRcv = END;
				}
				break;
			case END:
				stop = 1;
				break;
		}
	}
  // printf("\n\nEXITING ReceiveFrame\n\n");
  return 0;
}

/**
* Reads the data in a Information frame
* @param lk - LinkLayer's info struct
* @return 0 if there's no error, 1 for destuffing error and 2 for Bcc2 check error
*/
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
      lk->readBytes--; //Subtracted non data - wrong increment
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

  // printf("READ DTA\n");
  // int j;
  // for (j = 0; j < lk->frameSize; j++) {
  //   printf("%x\n", lk->frame[j]);
  // }

  return 0;
}

int bcc2Calc(unsigned char* buffer, int length){
  int i;
  unsigned char xorResult = buffer[0]; //D0

  for (i = 1; i < length; i++) {
    xorResult ^= buffer[i];
  }

  return xorResult;
}

int bcc2Check(LinkLayer* lk){
  int i;
  unsigned char xorResult = lk->frame[0]; //D0

  for (i = 1; i < lk->frameSize-1; i++) {
    xorResult ^= lk->frame[i];
  }
                  /*BCC2*/
  if(xorResult != lk->frame[lk->frameSize-1]){
    printf("->BCC2Check result: %x, instead in the frame was %x\n", xorResult, lk->frame[lk->frameSize-1]);
    return 1;
  }

  lk->frameSize--;//BCC2 Ignored (Deleted)
  lk->readBytes--;
  return 0; //BCC2 field is correct
}


int stuffing(unsigned char* buff, unsigned int* size){

  unsigned int i;

  for(i=0; i< *size;){
    if(buff[i] == FLAG){
      memmove(&buff[i+2], &buff[i+1], (*size) - (i+1));
      buff[i] = ESC;
      buff[i+1] = FLAG_EX;
      (*size) += 1;
      i+=2;
    }

    else if(buff[i] == ESC){
      memmove(&buff[i+2], &buff[i+1], (*size) - (i+1));
      buff[i+1] = ESC_EX;
      (*size) += 1;
      i+=2;
    }
    else
      i++;
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
  lk->readBytes -= deletedBytes;
  return 0;
}

int llclose(int fd){
  printf("Entered llclose %d\n", linkLayer.prog);
  unsigned char discMsg[] = {FLAG, ADDRESS, DISC, DISC ^ ADDRESS, FLAG};
  retryCount = 0;
  if(linkLayer.prog == RECEIVER){
    while(retryCount < N_TRIES){
      printf("Entrou Recetor\n");
      receiveFrame(&linkLayer);
      if(linkLayer.frame[C_IDX] == DISC){
        write(fd, discMsg, 5);
        alarm(TIMEOUT);
        receiveFrame(&linkLayer);
        if(linkLayer.frame[C_IDX] == UA){
          alarm(0);
          break;
        }
      }
    }
  }
  else{
    while(retryCount < N_TRIES){
      printf("Entrou\n");
      printf("TRANSMISSOR sent %lu\n", write(fd, discMsg, 5));
      alarm(TIMEOUT);
      receiveFrame(&linkLayer);
      printf("llclose read CONTROL %x\n", linkLayer.frame[C_IDX]);
      if(linkLayer.frame[C_IDX] == DISC){
        discMsg[2] = UA;
        discMsg[3] = UA ^ ADDRESS;
        write(fd, discMsg, 5);
        alarm(0);
        break;
      }
    }
  }
  if(retryCount == N_TRIES){
    printf("llclose::Exceeded number of tries\n");
    exit(1);
  }

  if ( tcsetattr(fd, TCSANOW, &oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }
  printf("Exiting llclose...\n");
  close(fd);
  return 0;
}
