#include "serialCom.h"

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
