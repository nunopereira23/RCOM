#include "appLayer.h"
#include "linkLayer.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>



int main(int argc, char** argv)
{

  if ( (argc < 3) ||
        ((strcmp("0", argv[1])!= 0) &&
        (strcmp("1", argv[1])!= 0)) ||
        ((strcmp("r", argv[2])!= 0) &&
        (strcmp("w", argv[2])!= 0))){
    printf("Usage:\tnSerial SerialPort number and desired operation ('r' or 'w')\n\tex: serialCom 0 r\n");
    exit(1);
  }

  if((strcmp("w", argv[2]) == 0) && argc < 4){
    printf("Usage ex. serialCom 0 w filePath\n");
  }


  struct sigaction sigact;
  sigact.sa_handler = alarmHandler;
  sigact.sa_flags = 0;

  if(sigaction(SIGALRM, &sigact, NULL) != 0){
    perror("Failed to install the signal handler\n");
    return -1;
  }

  	if(strcmp(argv[2], "r") == 0){
      AppLayer appLayer;
      linkLayer.prog = RECEIVER;

      if((appLayer.serialPortFD = llopen(atoi(&argv[1][0]) , RECEIVER)) < 0){
  				printf("Receiver failed to establish the connection\n");
          return 1;

      appLayer.packetSize = PACKET_SIZE;
      appLayer.packet = malloc(PACKET_SIZE);

      if(receiveStartPacket(&appLayer) != 0){
        printf("Didn't receive the start packet\n");
      }

      if(argc == 4)
        appLayer.fileName = argv[4];

      if((appLayer.fileFD = open(appLayer.fileName, O_CREAT | O_WRONLY)) < 0){
        printf("Couldn't create file named %s\n", appLayer.fileName);
        return 1;
      }

      receiveDataPacket(&appLayer);

      close(appLayer.fileFD);
  	}
  	else{
      linkLayer.prog = TRANSMISSOR;
  		if((linkLayer.fd = llopen(atoi(&argv[1][0]), TRANSMISSOR)) < 0){
  			printf("Transmissor failed to establish the connection fd %d\n", linkLayer.fd);
        return 1;
      }

      appLayer.fileName = argv[4];
      appLayer.packetSize = PACKET_SIZE;
      appLayer.packet = malloc(PACKET_SIZE);

      if((appLayer.fileFD = open(appLayer.fileName, O_RDONLY)) < 0){
        printf("Couldn't open file named %s\n", appLayer.fileName);
        return 1;
      }

      getFileSize(&appLayer);

      if(sendControlPacket(&appLayer, START_PACKET) != 0){
        printf("Couldn't send start packet\n");
      }


    }

  	return 0;
  }
}

int receiveStartPacket(AppLayer* appLayer){
  while(!llread(appLayer->serialPortFD, appLayer->packet))

  if(appLayer->packet[0] != START_PACKET)
      return 1;

  if(appLayer->packet[1] != T_SIZE)
    return 1;

  int i;
  appLayer->fileSize = 0;
  for(i = 0; i < appLayer->packet[2]; i++){
    appLayer->fileSize += appLayer->packet[i + 2];
  }


  if(appLayer->packet[2 + appLayer->packet[2]] != T_NAME)
      return 1;

  appLayer->fileName = malloc(appLayer->packet[3 + appLayer->packet[2]]);
  memcpy(appLayer->fileName, appLayer->packet + 4 + appLayer->packet[2], appLayer->packet[3 + appLayer->packet[2]]);

  return 0;
}

int sendControlPacket(AppLayer* appLayer, unsigned char control){
  unsigned char i;

  appLayer->packet[0] = control;
  appLayer->packet[1] = T_SIZE;


  unsigned char sizeNBytes = appLayer->fileSize / 255;
  if(appLayer->fileSize % 255){
    sizeNBytes++;

    for(i = 0; i < sizeNBytes-1; i++){
      appLayer->packet[3 + i] = 255;
    }
    appLayer->packet[3 + sizeNBytes] = appLayer->fileSize % 255;
  }
  else{
    for(i = 0; i < sizeNBytes; i++){
      appLayer->packet[3 + i] = 255;
    }
  }


  appLayer->packet[2] = sizeNBytes;
  appLayer->packet[sizeNBytes+1] = T_NAME;
  appLayer->packet[sizeNBytes+2] = strlen(appLayer->fileName)+1;

  memcpy(appLayer->packet + sizeNBytes+3, appLayer->fileName, strlen(appLayer->fileName)+1);

  return 0;
}

void getFileSize(AppLayer* appLayer){
  struct stat statBuf;

  fstat(appLayer->fileFD, &statBuf);
  appLayer->fileSize = statBuf.st_size;
}

int test(void){

  if(linkLayer.prog == TRANSMISSOR){
      linkLayer.seqNum = 0;
    unsigned char packect[] = "Tudo bem caro Duarte?";

    // printf("Antes Stuffing\n");
    // printf("Size %d\n", size);
    // int i;
    // for (i = 0; i < size; i++) {
    //   printf("%x\n", packect[i]);
    // }
    // printf("\n\n\n");
    // stuffing(packect, &size);
    //
    // printf("Apos Stuffing\n");
    // printf("Size %d\n", size);

    printf("llwrite wrote %d\n", llwrite(linkLayer.fd, packect, 22));
  }
  else{
    linkLayer.seqNum = 1;
    printf("llread read %d\n", llread(linkLayer.fd, linkLayer.frame));
    int i;
    for(i = 0; i < linkLayer.frameSize; i++){
      printf("Hex %x\n", linkLayer.frame[i]);
    }
    printf("Dados \"%s\"\n", linkLayer.frame);
  }
  return 0;
}
