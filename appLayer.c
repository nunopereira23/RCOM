#include "appLayer.h"
#include "linkLayer.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#define STATISCS


int main(int argc, char** argv)
{

  if ( (argc < 3) ||
        ((strcmp("0", argv[1])!= 0) &&
        (strcmp("1", argv[1])!= 0)) ||
        ((strcmp("r", argv[2])!= 0) &&
        (strcmp("w", argv[2])!= 0))){
    printf("Usage:\n\tserialCom <numPort> r [fileName]\n\tserialCom <numPort> w <fileName>\n");
    exit(1);
  }

  if((strcmp("w", argv[2]) == 0) && argc < 4){
    printf("Usage ex. serialCom 0 w filePath\n");
    exit(2);
  }

const unsigned int baudArray[] = {B1200, B2400, B4800, B19200, B38400, B115200};

unsigned int choice;

do{
  printf("Please choose the desired baudrate\n"
  "1-B1200 | 2-B2400 | 3-B4800 | 4-B19200 | 5-B38400 | 6-B115200\n"
  "Choice: ");
  scanf("%d", &choice);
}while(choice < 1 || choice > 6);
linkLayer.baudrate = baudArray[choice-1];

do{
  printf("Please insert the frame size (bytes) [1024 - 64000]\n"
        "Choice: ");
  scanf("%d", &choice);
}while(choice < 512 || choice > 64000);
FRAME_SIZE = choice;

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

    PACKET_SIZE = FRAME_SIZE-6;
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

    printf("Received %u Bytes\n", receiveFile(&appLayer));

    close(appLayer.fileFD);
    llclose(appLayer.serialPortFD);

	}
	else{

    linkLayer.prog = TRANSMISSOR;
		if((appLayer.serialPortFD = llopen(atoi(argv[1]), TRANSMISSOR)) < 0){
			printf("Transmissor failed to establish the connection fd %d\n", appLayer.serialPortFD);
      return 1;
    }

    linkLayer.seqNum = 0;

    appLayer.fileName = argv[3];
    PACKET_SIZE = FRAME_SIZE-6;
    appLayer.packet = malloc(PACKET_SIZE);

    if((appLayer.fileFD = open(appLayer.fileName, O_RDONLY)) < 0){
      printf("Couldn't open file named %s\n", appLayer.fileName);
      return 1;
    }

    getFileSize(&appLayer);

    if(sendControlPacket(&appLayer, START_PACKET) != 0){
      printf("Couldn't send start packet\n");
    }

    printf("Sent %d bytes from file\n", sendFile(&appLayer));

    free(appLayer.packet);
    llclose(appLayer.serialPortFD);
    close(appLayer.fileFD);
  }
  return 0;
}

unsigned int receiveFile(AppLayer* appLayer){
  unsigned int readBytes = 0;
struct timespec start, end;
clock_gettime(CLOCK_REALTIME, &start);
  while(1){
      appLayer->packetSize =  llread(appLayer->serialPortFD, appLayer->packet);

    if(appLayer->packetSize != 0){
      if(appLayer->packet[0] == END_PACKET)
        break;

      readBytes += appLayer->packetSize-4;
      write(appLayer->fileFD, appLayer->packet + 4, appLayer->packetSize-4);
    }
}
clock_gettime(CLOCK_REALTIME, &end);
printf("Time elapsed: %lu s\n", getElapsedTimeSecs(&start, &end));
#ifdef STATISCS
  printf("Tf = %lu s", getElapsedTimeSecs(&start, &end)/((appLayer->fileSize *8 ) / PACKET_SIZE));
#endif
  return readBytes;
}

unsigned int sendFile(AppLayer* appLayer){
  unsigned int writtenBytes = 0;
  int readFromFile  = 1;
  int llwriteReturn;

  appLayer->packet[0] = DATA_PACKET;
  appLayer->packet[1] = 0;
  appLayer->packet[2] = (PACKET_SIZE - 4) / 256;
  appLayer->packet[3] = (PACKET_SIZE - 4) % 256;


  readFromFile = read(appLayer->fileFD, appLayer->packet + 4, PACKET_SIZE-4);
  while(readFromFile){
      appLayer->packet[1] = (appLayer->packet[1] + 1) % 256;
      llwriteReturn = llwrite(appLayer->serialPortFD, appLayer->packet, readFromFile+4);

       if(llwriteReturn-4){
          readFromFile =  read(appLayer->fileFD, appLayer->packet + 4, PACKET_SIZE-4);
          writtenBytes += llwriteReturn - 4;
      }
  }
   sendControlPacket(appLayer, END_PACKET);
   return writtenBytes;
}

int receiveStartPacket(AppLayer* appLayer){
  while(!(appLayer->packetSize = llread(appLayer->serialPortFD, appLayer->packet)));

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

unsigned long getElapsedTimeSecs(struct timespec* start, struct timespec* end){
  return (end->tv_sec + end->tv_nsec/1000000000) - (start->tv_sec + start->tv_nsec/1000000000);
}
