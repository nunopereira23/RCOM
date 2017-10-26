/*Serial COM*/
#include "linkLayer.h"

int main(int argc, char** argv)
{

  if ( (argc < 3) ||
        (strcmp("0", argv[1])!= 0 &&
        strcmp("1", argv[1])!= 0) ||
        (strcmp("r", argv[2])!= 0 &&
        strcmp("w", argv[2])!= 0) ){
    printf("Usage:\tnSerial SerialPort and desired operation ('r' or 'w')\n\tex: serialCom /dev/ttyS0 r\n");
    exit(1);
  }

  struct sigaction sigact;
  sigact.sa_handler = alarmHandler;
  sigact.sa_flags = 0;

  if(sigaction(SIGALRM, &sigact, NULL) != 0){
    perror("Failed to install the signal handler\n");
    return -1;
  }

	if(strcmp(argv[2], "r") == 0){
    linkLayer.prog = RECEIVER;
		if((linkLayer.fd = llopen(argv[1][0] - '0', RECEIVER)) < 0){
				printf("Receiver failed to establish the connection\n");
		}
	}
	else{
    linkLayer.prog = TRANSMISSOR;
		if((linkLayer.fd = llopen(argv[1][0] - '0', TRANSMISSOR)) < 0)
			printf("Transmissor failed to establish the connection\n");
  }

	return 0;
}
