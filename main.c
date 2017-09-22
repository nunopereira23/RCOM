/*Serial COM*/
#include "serialCom.h"

int main(int argc, char** argv)
{

    struct termios oldtio,newtio;


    if ( (argc < 3) ||
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) &&
  	      (strcmp("/dev/ttyS1", argv[1])!=0) &&
          strcmp("r", argv[2])!=0 &&
          strcmp("w", argv[2])!=0)
        ) {
      printf("Usage:\tnSerial SerialPort and desired operation ('r' or 'w')\n\tex: nserial /dev/ttyS1 r\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


    serialP = open(argv[1], O_RDWR | O_NOCTTY );
    if (serialP <0) {
      perror(argv[1]); exit(-1);
    }

    if ( tcgetattr(serialP,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio,STOP sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    // newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    // newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */
    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */




  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(serialP, TCIOFLUSH);

    if ( tcsetattr(serialP,TCSrindexANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    if(argv[2] == 'r')
      receive();
    else
      transmit();
    }


    tcsetattr(serialP,TCSANOW,&oldtio);
    close(serialP);
    return 0;
}
