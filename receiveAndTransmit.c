#include "serialCom.h"

int receive(void){
  volatile int STOP = FALSE;
  unsigned int nBytes = 0;
  char buf[255];

  while (STOP==FALSE) {       /* loop for input */
    nBytes = read(serialP,buf,255);
    if (index(buf, '\0') != NULL)
     STOP=TRUE;
    buf[nBytes]=0;
    printf(":%s:%d\n", buf, nBytes); /* so we can printf... */
  }

  return 0;
}

int transmit(void){
  char buf[255];
  int stdinCopy = dup(STDIN_FILENO);

  if(dup2(STDIN_FILENO, serialP) == -1)
    perror("Failed redirecting stdin to the serial port");

    read(STDIN_FILENO, buf, sizeof(buf));
    while(index(buf, '\0') != NULL){
      read(STDIN_FILENO, buf, sizeof(buf));
    }

    //Restoring stdin to its original state
    dup2(stdinCopy, STDIN_FILENO);

  return 0;
}
