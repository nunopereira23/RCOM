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

  AppLayer appLayer;
	if(strcmp(argv[2], "r") == 0){

    linkLayer.prog = RECEIVER;

    if((appLayer.serialPortFD = llopen(atoi(argv[1]) , RECEIVER)) < 0){
				printf("Receiver failed to establish the connection\n");
        return 1;
    }

    // test();
    // return 0;

    appLayer.packetSize = PACKET_SIZE; //Does nothing
    appLayer.packet = malloc(PACKET_SIZE);

    linkLayer.seqNum = 1;

    if(receiveStartPacket(&appLayer) != 0){
      printf("Didn't receive the start packet\n");
    }

    if(argc == 4)
      appLayer.fileName = argv[3];

    if((appLayer.fileFD = open(appLayer.fileName, O_CREAT | O_WRONLY, 0666)) < 0){
      printf("Couldn't create file named %s\n", appLayer.fileName);
      return 1;
    }

    receiveFile(&appLayer);

    close(appLayer.fileFD);
	}
	else{

    linkLayer.prog = TRANSMISSOR;
		if((appLayer.serialPortFD = llopen(atoi(argv[1]), TRANSMISSOR)) < 0){
			printf("Transmissor failed to establish the connection fd %d\n", appLayer.serialPortFD);
      return 1;
    }

    // test();
    // return 0;

    linkLayer.seqNum = 0;

    appLayer.fileName = argv[3];
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

    sendFile(&appLayer);

    llclose(appLayer.serialPortFD);
    close(appLayer.fileFD);
  }
  return 0;
}

unsigned int receiveFile(AppLayer* appLayer){
  unsigned int readBytes = 0;
  char stop = 0;
  while(!stop){
    appLayer->packetSize =  llread(appLayer->serialPortFD, appLayer->packet);
    readBytes += appLayer->packetSize-4;
    if(appLayer->packet[0] != END_PACKET)
      stop = 1;

    write(appLayer->fileFD, appLayer->packet + 4, appLayer->packetSize-4);
  }
  return readBytes;
}

unsigned int sendFile(AppLayer* appLayer){
  unsigned int writtenBytes = 0;
  unsigned int justRead  = 1;

  appLayer->packet[0] = DATA_PACKET;
  appLayer->packet[1] = 0;
  appLayer->packet[2] = (PACKET_SIZE - 4) / 256;
  appLayer->packet[3] = (PACKET_SIZE - 4) % 256;


  read(appLayer->fileFD, appLayer->packet + 4, PACKET_SIZE-4);
  while(justRead){
      appLayer->packet[1] = (appLayer->packet[1] + 1) % 256;
       if((writtenBytes += llwrite(appLayer->serialPortFD, appLayer->packet, PACKET_SIZE) - 4))
          justRead =  read(appLayer->fileFD, appLayer->packet + 4, PACKET_SIZE-4);
  }
   sendControlPacket(appLayer, END_PACKET);
   return writtenBytes;
}

int receiveStartPacket(AppLayer* appLayer){
  while(!(appLayer->packetSize = llread(appLayer->serialPortFD, appLayer->packet)));

  printf("\n\n------------AppLayer------\n");
  int j;
  for (j = 0; j < appLayer->packetSize; j++) {
    printf("%x\n", appLayer->packet[j]);
  }


  if(appLayer->packet[0] != START_PACKET)
      return 1;

  if(appLayer->packet[1] != T_SIZE)
    return 1;

  int i;
  appLayer->fileSize = 0;
  for(i = 0; i < appLayer->packet[2]; i++){
    appLayer->fileSize += appLayer->packet[i + 2];
  }


  if(appLayer->packet[3 + appLayer->packet[2]] != T_NAME)
      return 1;

  appLayer->fileName = malloc(appLayer->packet[4 + appLayer->packet[2]]);
  memcpy(appLayer->fileName, appLayer->packet + 5 + appLayer->packet[2], appLayer->packet[4 + appLayer->packet[2]]);

  return 0;
}

int sendControlPacket(AppLayer* appLayer, unsigned char control){
  unsigned char i;

  appLayer->packet[0] = control;
  appLayer->packet[1] = T_SIZE;



  unsigned char sizeNBytes = appLayer->fileSize / 256;
  printf("FileSize/256 = %d\n", sizeNBytes);
  if(appLayer->fileSize % 256){
    sizeNBytes++;

    for(i = 0; i < sizeNBytes-1; i++){
      appLayer->packet[3 + i] = 255;
    }
    appLayer->packet[3 + sizeNBytes-1] = appLayer->fileSize % 256;
  }
  else{
    for(i = 0; i < sizeNBytes; i++){
      appLayer->packet[3 + i] = 255;
    }
  }


  appLayer->packet[2] = sizeNBytes;
  appLayer->packet[sizeNBytes+3] = T_NAME;
  unsigned int fileNameSize = strlen(appLayer->fileName)+1;
  appLayer->packet[sizeNBytes+4] = fileNameSize;


  memcpy(appLayer->packet + sizeNBytes+5, appLayer->fileName, fileNameSize);

  appLayer->packetSize = 5 + sizeNBytes + fileNameSize;

 if(!llwrite(appLayer->serialPortFD, appLayer->packet, appLayer->packetSize))
       return 1;
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
