#include "serialCom.h"
#include "stateMachines.h"
#define TRANSMISSOR 1
#define RECEIVER 0
int retryCount;

int receive(void){
  volatile int STOP = FALSE;
  unsigned int nBytes = 0;
  char buf[255];

  while (STOP==FALSE) {       /* loop for input */
    bzero(buf, sizeof(buf));
    nBytes = read(serialP, buf,sizeof(buf));
    buf[nBytes] = 0;
    printf(":%s:%d\n", buf, nBytes); /* so we can printf... */
    if (buf[0] == '\n')
     STOP=TRUE;
   }
   return 0;
}

int transmit(void){
  char buf[255];
  unsigned int nBytes;


      while (buf[0] != '\n') {
      nBytes = read(STDIN_FILENO, buf, sizeof(buf));
      printf("%d\n", nBytes);
      buf[nBytes-1] = '\0';
      write(serialP, buf, strlen(buf)+1);
    }

    //Restoring stdin to its original state
    // dup2(stdinCopy, STDIN_FILENO);

  return 0;
}

void* alarmHandler(int sigNum){
  retryCount++;
}


/**
* @ Establishes a serial connection between two machines
* @ parm port -
* @ parm flag - boolean flag, 0 for emmisor and 1 stands for receiver
*/
int llopen(int port, char flag){
  unsigned int retryCount = 0, remainder;

  if(flag != TRANSMISSOR && flag != RECEIVER){
    perror("llopen()::Couldn't open serialPort fd\n");
    return -1;
  }
  //TODO test if port is between range
  char path = "/dev/ttyS", portString[2];

  portString[0] =  port + '0';
  portString[1] = '\0';

  strcat(path, &portString);

  if((serialPort = open(path, O_RDWR | O_NOCTTY )) < 0){
    perror("llopen()::Couldn't open serialPort fd\n");
    return -1;
  }

  if(flag == TRANSMISSOR){
    int disconnect = 0, state = 0;
    struct sigaction sigact;
    sigact.sa_handler = alarmHandler;
    sigact.sa_flags = 0;

    if(sigaction(SIGALARM, &sigact, NULL) != 0){
      perror("llopen()::receiver failed to install the signal handler\n");
      return -1;
    }

    while(!disconnect){
      switch (state) {
        case  SET_STATE:
        transmit(path, SET); //Send set message
        if( (remainder = alarm(3)) != 0){
          perror("Alarm already scheduled in %d seconds\n", remainder);
        }
        break;
        case  WAIT_UA:

        break;

      }
    }

  }

  else{

  }
}
