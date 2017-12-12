#include "ftpDownloader.h"

int main(int argc, char* argv[]){
  if(argc != 2){
    printf("Expected 1 argument: ftp://[<user>:<password>@]<host>/<url-path>\n");
    return 1;
  }

  FTP ftp;

  parseUrl(argv[1], &ftp);
  findFileName(ftp.path, &ftp.fileName);

  #ifdef DEBUG
    printf("FileName: %s\n", ftp.fileName);
  #endif

  if(establishCmdConnection(&ftp)){
      exit(1);
  }

  char cmdBuff[CMD_BUFF_LEN];
  receiveCmdResponse(&ftp, cmdBuff);

  //USER username
  writeCmd(&ftp, (char*[]){"USER ", ftp.username, NULL});
  receiveCmdResponse(&ftp, cmdBuff);
    
  //PASS password
  writeCmd(&ftp, (char*[]){"PASS ", ftp.password, NULL});
  receiveCmdResponse(&ftp, cmdBuff);

  //Enter Passive mode
  writeCmd(&ftp, (char*[]){"PASV", NULL});
  receiveCmdResponse(&ftp, cmdBuff);
  //Example Response 227 Entering Passive Mode (90,130,70,73,91,143).

  int dataPort = 0;
  char ipAddress[30], ipFrag1[4], ipFrag2[4], ipFrag3[4], ipFrag4[4], portFrag1[10], portFrag2[10]; //Potencially a problem
  sscanf(cmdBuff, "%*[^(](%[^,],%[^,],%[^,],%[^,],%[^,],%[^)]%*[)]%*[.]%*[\n]", ipFrag1, ipFrag2, ipFrag3, ipFrag4, portFrag1, portFrag2);

  dataPort = 256 * atoi(portFrag1) + atoi(portFrag2);
  #ifdef DEBUG
    printf("PASV response \"%s\"", cmdBuff);
    printf("IPFrag1: %s\n", ipFrag1);
    printf("IPFrag2: %s\n", ipFrag2);
    printf("IPFrag3: %s\n", ipFrag3);
    printf("IPFrag4: %s\n", ipFrag4);
    printf("PortFrag1: %s\n", portFrag1);
    printf("PortFrag2: %s\n", portFrag2);    
    printf("Data Port - %d\n", dataPort);
  #endif

  addressCat(ipFrag1, ipFrag2, ipFrag3, ipFrag4, ipAddress);
  if(establishDataConnection(&ftp, ipAddress, dataPort)){
    exit(1);
  }

  //Set Transfer type to binary mode
  writeCmd(&ftp, (char*[]){"TYPE ", "I", NULL});
  receiveCmdResponse(&ftp, cmdBuff);

  // //Download File
  writeCmd(&ftp, (char*[]){"RETR ", ftp.path, NULL});
  receiveCmdResponse(&ftp, cmdBuff);


  receiveFile(&ftp);

  //Exit
  writeCmd(&ftp, (char*[]){"QUIT", NULL});
  receiveCmdResponse(&ftp, cmdBuff);

  close(ftp.dataFD);
  close(ftp.cmdFD);
  free(ftp.username);
  free(ftp.password);
  return 0;
}


void receiveFile(FTP* ftp){
  char dataBuff[1024];
  int fileFD;
  int readBytes;

  if((fileFD = open(ftp->fileName, O_CREAT | O_WRONLY, 0666)) < 0){
    printf("receiveFile::Error creating file named %s\n", ftp->fileName);
  }

  while((readBytes = read(ftp->dataFD, dataBuff, sizeof(dataBuff)))){
      write(fileFD, dataBuff, readBytes);
  }
  close(fileFD);
}



void findFileName(char* pathName, char** fileName){
	char path[strlen(pathName) + 1];
	memcpy(path, pathName, strlen(pathName) + 1);

  char* lastSlashPos = path;
  char* slashPos =  strtok(path, "/");
  while((slashPos = strtok(NULL, "/"))){
    lastSlashPos = slashPos;
  }

  // printf("\t%s\n", lastSlashPos);

  *fileName = malloc(strlen(lastSlashPos) + 1);
  strcpy(*fileName, lastSlashPos);
}


int establishCmdConnection(FTP* ftp){
  struct sockaddr_in server_addr;
  
  bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = ftp->h->h_addrtype; //The type of address being returned; usually AF_INET.
	server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)ftp->h->h_addr)));	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_CMD_PORT);		/*server TCP port must be network byte ordered */

	if ((ftp->cmdFD = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()::Failed opening TCP socket for FTP commands");
        	return 1;
  }
  
  if(connect(ftp->cmdFD, (struct sockaddr *)&server_addr,  sizeof(server_addr)) < 0){
      perror("connect()::cmd connection");
      return 1;
  }

  return 0;
}

int establishDataConnection(FTP* ftp, char* ipAddress, int dataPort){
   	struct sockaddr_in server_addr;
  
	ftp->h = gethostbyname(ipAddress);

	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = ftp->h->h_addrtype; //The type of address being returned; usually AF_INET.
	server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)ftp->h->h_addr)));	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(dataPort);		/*server TCP port must be network byte ordered */


	if ((ftp->dataFD = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    perror("socket()::Failed opening TCP socket for FTP data");
    return 1;
  }
  
  if(connect(ftp->dataFD, (struct sockaddr *)&server_addr,  sizeof(server_addr)) < 0){
    perror("connect()::data connection");
    return 1;
  }

  return 0;
}

void receiveCmdResponse(FTP* ftp, char* cmdBuff){
  bzero(cmdBuff, CMD_BUFF_LEN);
	read(ftp->cmdFD, cmdBuff, CMD_BUFF_LEN-1);

  if((cmdBuff[0] - '0' != POSITIVE_COMP_REPLY) && 
    (cmdBuff[0] - '0' != POSITIVE_INT_REPLY) && 
    cmdBuff[0] - '0' != POSITIVE_PRE_REPLY){
    printf("%s", cmdBuff);
    printf("receiveCmdResponse::Server response error\n");
    exit(2);
  }
  
  char endControl[5];
  memcpy(endControl, cmdBuff, 3);
  endControl[3] = '\0';
  strcat(endControl, " ");//ControlCode<SP>

  #ifdef DEBUG
    printf("EndControl(%lu) \"%s\"\n", strlen(endControl), endControl);
  #endif

  while(strncmp(cmdBuff, endControl, 4) != 0){
    printf("%s", cmdBuff);
    readLine(ftp, cmdBuff);
	}
  printf("%s", cmdBuff);
}

void readLine(FTP* ftp, char* buffer){
  char byte = 0;
  int i = 0;
  while(byte != '\n' && byte != '\r'){
    read(ftp->cmdFD, &byte, 1);
    buffer[i] = byte;
    i++;
  }

  if(byte == '\r'){
    buffer[i-1] = '\n';
    read(ftp->cmdFD, &byte, 1);
  }
  buffer[i] = '\0';
  
}

void writeCmd(FTP* ftp, char** cmdArgs){
  char buff[CMD_BUFF_LEN];
  memset(buff, 0 ,CMD_BUFF_LEN);

  int i;
  for(i = 0; cmdArgs[i] != NULL; i++){
    #ifdef DEBUG
      printf("Cmd ARG(%d): %s\n", i, cmdArgs[i]);
    #endif
    strcat(buff, cmdArgs[i]);
  }

  strcat(buff, "\n"); //The server reads the command until a newline

  #ifdef DEBUG
    printf("writeCmd::FullCMD(%lu): %s\n", strlen(buff), buff);
  #endif

  write(ftp->cmdFD, buff, strlen(buff));
}

void addressCat(char* ipFrag1, char* ipFrag2, char* ipFrag3, char* ipFrag4, char* ipAddress){
  memset(ipAddress, 0, 30);
  strcat(ipAddress, ipFrag1);
  strcat(ipAddress, ".");
  strcat(ipAddress, ipFrag2);
  strcat(ipAddress, ".");
  strcat(ipAddress, ipFrag3);
  strcat(ipAddress, ".");
  strcat(ipAddress, ipFrag4);

  #ifdef DEBUG
    printf("adressCat::ipAddress: \"%s\"\n", ipAddress);
  #endif
}
