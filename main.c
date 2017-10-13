/*Serial COM*/
#include "serialCom.h"

int main(int argc, char** argv)
{

  if ( (argc < 3) ||
       ((strcmp("0", argv[1])!= 0 &&
        strcmp("1", argv[1])!= 0) &&
        strcmp("r", argv[2])!= 0 &&
        strcmp("w", argv[2])!= 0) ) {
    printf("Usage:\tnSerial SerialPort and desired operation ('r' or 'w')\n\tex: serialCom /dev/ttyS0 r\n");
    exit(1);
  }

    if(strcmp(argv[2], "r") == 0)
      llopen(argv[1][0] - '0', RECEIVER);
    else
      llopen(argv[1][0] - '0', TRANSMISSOR);
    return 0;
}
