#include "serialCom.h"

int receive(void){
  volatile int STOP = FALSE;
  unsigned int nBytes = 0;
  char buf[255];
  int c;

  while (STOP==FALSE) {       /* loop for input */
    it += read(serialP, &c, 1);

    strcat(buf, &c);
    printf(":%s:%d\n", buf, nBytes); /* so we can printf... */
    if (buf[it] == '\0')
     STOP=TRUE;
    else{
      if(it == 255){
        it = 0;
        bzero(buf, sizeof(buf));
     }
    }
  }

  return 0;
}

int transmit(void){
  char buf[255];
  //int stdinCopy = dup(STDIN_FILENO);

  // if(dup2(STDIN_FILENO, serialP) == -1)
  //   perror("Failed redirecting stdin to the serial port");
    //printf("Size buf: %lu\n", sizeof(buf));
    read(STDIN_FILENO, buf, sizeof(buf));
    while(index(buf, '\0') != NULL){
      read(STDIN_FILENO, buf, sizeof(buf));
      write(serialP, buf, strlen(buf));
    }

    //Restoring stdin to its original state
    // dup2(stdinCopy, STDIN_FILENO);

  return 0;
}
