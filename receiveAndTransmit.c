#include "serialCom.h"
#include "stateMachines.h"

unsigned int retryCount, state;

int receive(int fd, char message[], unsigned int size){
  volatile int stop = FALSE;
  unsigned int nBytes = 0;
  char buf[255];

  while (stop==FALSE) {       /* loop for input */
    bzero(buf, sizeof(buf));
    nBytes = read(serialP, buf,sizeof(buf));
    buf[nBytes] = 0;
    //printf(":%s:%d\n", buf, nBytes); /* so we can printf... */
    if (buf[0] == '\n')
     stop=TRUE;
   }
   return 0;
}

// int transmit(int fd, char message[], unsigned int size){
//   char buf[255];
//   unsigned int nBytes;
//
//
//   while (buf[0] != '\n') {
//     nBytes = read(STDIN_FILENO, buf, sizeof(buf));
//     //printf("%d\n", nBytes);
//     buf[nBytes-1] = '\0';
//     write(serialP, buf, strlen(buf)+1);
//   }
//     return 0;
// }

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
  char byte;

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
    printf("llopen()::Couldn't open serialPort %d\n", port);
    return -1;
  }


  struct termios oldtio, newtio;


/*
  Open serial port device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
*/

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

  // newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  // newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */
  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

  tcflush(serialPort, TCIOFLUSH);

  if ( tcsetattr(serialPort,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
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

    state = SET_SEND;

    while(!connected &&  retryCount != N_TRIES){
      if(state != SET_SEND)
        read(serialPort, &byte, 1);
      switch (state) {
        case  SET_SEND:
          message[0] = FLAG;
          message[1] = ADDRESS;
          message[2] = SET;
          message[4] = SET ^ ADDRESS;
          message[5] = FLAG;

          write(serialPort, message, 5); //Send set message
          if(alarm(3) != 0){
            printf("Alarm already scheduled in seconds\n");
          }

          //WAIT FOR UA
          state = START;
          break;
        case  START://UA State Machine
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
            if(byte == UA)
              state = UA_RCV;
            else if(byte == FLAG)
              state = FLAG_RCV;
            else
              state = START;
          break;

          case UA_RCV:
            if(byte == (ADDRESS ^ UA))
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
      unsigned int conEstab = 0;
      state = START;
      while(!conEstab){
        read(serialPort, &byte, 1);
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
              state = STOP;
            else
              state = START;
          break;

          case STOP:
            conEstab = TRUE;
        }
      }

      //After a successful SET message was received, send a UA
      message[0] = FLAG;
      message[1] = ADDRESS;
      message[2] = UA;
      message[4] = UA ^ ADDRESS;
      message[5] = FLAG;
      if(write(serialPort, message, 5) == 0)
        printf("Failed to transmit an UA\n");
  }
  return serialPort;
}
