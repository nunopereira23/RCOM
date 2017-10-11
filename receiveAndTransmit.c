#include "serialCom.h"
#include "stateMachines.h"

unsigned int retryCount, state = SET_SEND;

int receive(int fd, char message[], unsigned int size){
  volatile int STOP = FALSE;
  unsigned int nBytes = 0;
  char buf[255];

  while (STOP==FALSE) {       /* loop for input */
    bzero(buf, sizeof(buf));
    nBytes = read(serialP, buf,sizeof(buf));
    buf[nBytes] = 0;
    //printf(":%s:%d\n", buf, nBytes); /* so we can printf... */
    if (buf[0] == '\n')
     STOP=TRUE;
   }
   return 0;
}

int transmit(int fd, char message[], unsigned int size){
  char buf[255];
  unsigned int nBytes;


  while (buf[0] != '\n') {
    nBytes = read(STDIN_FILENO, buf, sizeof(buf));
    //printf("%d\n", nBytes);
    buf[nBytes-1] = '\0';
    write(serialP, buf, strlen(buf)+1);
  }
    return 0;
}

void alarmHandler(int sigNum){
  retryCount++;
  state = SET_SEND;
}


/**
* @ Establishes a serial connection between two machines
* @ parm port -
* @ parm flag - boolean flag, 0 for emmisor and 1 stands for receiver
*/
int llopen(int port, char flag){
  int serialPort;
  char message[5];

  if(flag != TRANSMISSOR && flag != RECEIVER){
    perror("llopen()::Couldn't open serialPort fd\n");
    return -1;
  }
  //TODO test if port is between range
  char path[] = "/dev/ttyS", portString[2];

  portString[0] =  port + '0';
  portString[1] = '\0';

  strcat(path, portString);

  if((serialPort = open(path, O_RDWR | O_NOCTTY )) < 0){
    perror("llopen()::Couldn't open serialPort fd\n");
    return -1;
  }

  if(flag == TRANSMISSOR){
    int connected = 0;
    struct sigaction sigact;
    sigact.sa_handler = alarmHandler;
    sigact.sa_flags = 0;

    if(sigaction(SIGALRM, &sigact, NULL) != 0){
      perror("llopen()::receiver failed to install the signal handler\n");
      return -1;
    }

    while(!connected &&  retryCount != N_TRIES){
      switch (state) {
        case  SET_SEND:
          message[0] = FLAG;
          message[1] = ADDRESS;
          message[2] = SET;
          message[4] = SET ^ ADDRESS;
          message[5] = FLAG;

          transmit(serialPort, message, 5); //Send set message
          if(alarm(3) != 0){
            printf("Alarm already scheduled in seconds\n");
          }

          //WAIT FOR UA
          state = FLAG;
          break;
        case  FLAG://UA
          //        receive(serialPort, );

        case UA_RCV:
          alarm(0); //Disables the alarm
          connected = TRUE;
        break;

      }
    }
    if(retryCount == N_TRIES){
      printf("llopen failed due to the number o retries reaching it's limit\n");
      return -1;
    }

  }

  else{
      char byte;
      unsigned int conEstab = 0;
      while(!conEstab){
        receive(serialPort, &byte, 5)
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
            if(byte == ADDRESS ^ SET)
              state = BCC_OK;
            else if(byte == FLAG)
              state = FLAG_RCV;
            else
              state = START;
          break;

          case BCC_OK:
            if(byte == FLAG)
              state = STOP;
            else
              state = START;
          break;

          case STOP:
            conEstab = TRUE;
        }
      }

      //After a successful SET message was received, send a UA
      message[] = {FLAG, ADDRESS, UA, UA ^ ADDRESS, FLAG};
      if(transmit(serialPort, message) != 0)
        printf("Failed to transmit an UA\n");
  }
  return serialPort;
}
