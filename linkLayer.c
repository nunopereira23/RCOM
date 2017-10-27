#include "linkLayer.h"
#include "stateMachines.h"

unsigned int retryCount, state, stateRcv;

/**
* @ llopen() SigAlm handler, it increments nTries
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
* @ parm port -
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


  //printf("path %s\n", path);
  if((linkLayer.fd = open(path, O_RDWR | O_NOCTTY )) < 0){
    printf("llopen()::Couldn't open serialPort %d\n", port);
    return -1;
  }


  struct termios oldtio, newtio;

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

  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
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
  linkLayer.frame[3] = linkLayer.seqNum ^ ADDRESS;
  memmove(linkLayer.frame+4, buffer, length);
  linkLayer.frame[4+length] = bcc2;
  stuffing(linkLayer.frame + 4, &(lenghtStuffng));
  linkLayer.frameSize = length + 6;
  linkLayer.frame[linkLayer.frameSize-1] = FLAG;

  /*int i;
  for (i =0; i < linkLayer.frameSize; i++) {
    printf("Frame idx %i - %x\n", i, linkLayer.frame[i]);
  }*/

  unsigned char frameCpy[linkLayer.frameSize];
  memmove(frameCpy, linkLayer.frame, linkLayer.frameSize);

  state=START;

  unsigned char sent = 0, bytesWritten = 0;
  while(!sent && retryCount < N_TRIES){
    switch (state) {
      printf("llWrite state %d\n", state);
      case START:
        memmove(linkLayer.frame, frameCpy, linkLayer.frameSize);
        bytesWritten = write(fd, linkLayer.frame, linkLayer.frameSize);
        printf("BytesWritten %d\n", bytesWritten);
        alarm(TIMEOUT);
        state = RECEIVE;
      break;
      case RECEIVE:
        if(receiveFrame(&linkLayer))
          printf("llwrite: expected control frame but received Info instead\n");
        if(linkLayer.frame[C_IDX] == RR(0) || linkLayer.frame[C_IDX] == RR(1))
          state = END;
        else if(linkLayer.frame[C_IDX] == REJ(0) || linkLayer.frame[C_IDX] == REJ(1))
          state = START;
        else if(linkLayer.frame[C_IDX] == DISC)
          return callDisk;
      break;

      case END:
        alarm(0);
        sent = 1;
      break;
    }
  }
  return bytesWritten - 6; //FRAME_HEADER
}

int llread(int fd, unsigned char * buffer){
  buffer = linkLayer.frame;
  unsigned char message[] = {FLAG, ADDRESS, 0, 0, FLAG};

    char response = receiveFrame(&linkLayer);
    printf("%c", response);
    message[2] = response;
    message[3] = response ^ ADDRESS;
    write(fd, message, 5);

  return linkLayer.readBytes;
}

int possibleControlField(char controlField){
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
    printf("stateRcv %d\n", stateRcv);
		switch (stateRcv) {
			case START:
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
				break;

				if(lkLayer->frame[i] == SEQ_NUM0 || lkLayer->frame[i] == SEQ_NUM1){
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

int bcc2Calc(unsigned char* buffer, int length){
  int i;
  char xorResult = buffer[0]; //D0

  for (i = 1; i < length; i++) {
    xorResult ^= buffer[i];
  }

  return xorResult;
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


int stuffing(unsigned char* buff, unsigned int* size){

  unsigned int i;

  for(i=0; i< *size; i++){
    if(buff[i] == FLAG){
      memmove(&buff[i+2], &buff[i+1], (*size) - (i+1));
      buff[i] = ESC;
      buff[i+1] = FLAG_EX;
      size++;
    }

    if(buff[i] == ESC){
      memmove(&buff[i+2], &buff[i+1], (*size) - (i+1));
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

int llclose(int fd){
  unsigned char discMsg[] = {FLAG, ADDRESS, DISC, DISC ^ ADDRESS, FLAG};
  retryCount = 0;
  if(linkLayer.prog == TRANSMISSOR){
    while(retryCount < N_TRIES){
      write(fd, discMsg, 5);
      alarm(TIMEOUT);
      receiveFrame(&linkLayer);
      if(linkLayer.frame[C_IDX] == DISC){
        discMsg[2] = UA;
        discMsg[3] = UA ^ ADDRESS;
        write(fd, discMsg, 5);
        alarm(0);
        return 0;
      }
    }
    return -1;
  }
  else{
    while(retryCount < N_TRIES){
      write(fd, discMsg, 5);
      alarm(TIMEOUT);
      receiveFrame(&linkLayer);
      if(linkLayer.frame[C_IDX] == UA){
        alarm(0);
        return 0;
      }
    }
  return -1;
  }
}
