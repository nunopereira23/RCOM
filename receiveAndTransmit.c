volatile int STOP=FALSE;

int receive(void){
  unsigned int nBytes = 0;
  char buf[255];

  while (STOP==FALSE) {       /* loop for input */
    nBytes = read(serialP,buf,255);   /* returns after 5 chars have been input */
    if (index(buf, '\0') != NULL)
     STOP=TRUE;
    buf[nBytes]=0;               /* so we can printf... */
    printf(":%s:%d\n", buf, nBytes);
  }

  return 0;
}

int transmit(void){
  if(dup2(STDIN_FILENO, serialP) == -1)
    perror("Failed redirecting stdin to the serial port");

  return 0;
}
