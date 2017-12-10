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

  char cmdBuff[100];
  receiveCmdResponse(&ftp, cmdBuff);
  //receiveCmdResponse(&ftp, cmdBuff);

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



  close(ftp.dataFD);
  close(ftp.cmdFD);
  return 0;
}

void findFileName(char* pathName, char** fileName){
  char* lastSlashPos = pathName;
  char* slashPos =  strtok(pathName, "/");
  while((slashPos = strtok(NULL, "/"))){
    lastSlashPos = slashPos;
  }

  // printf("\t%s\n", lastSlashPos);

  *fileName = malloc(strlen(lastSlashPos) + 1);
  strcpy(*fileName, lastSlashPos);
}


int establishCmdConnection(FTP* ftp){
   struct	sockaddr_in server_addr;
  
  bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = ftp->h->h_addrtype; //The type of address being returned; usually AF_INET.
	server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)ftp->h->h_addr)));	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_CMD_PORT);		/*server TCP port must be network byte ordered */

  // #ifdef DEBUG
  //   printf("establishCmdConnection::Host's Ip: %s\n", inet_ntoa(*((struct in_addr *)ftp->h->h_addr)));
  // #endif

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

void receiveCmdResponse(FTP* ftp, char* cmdBuff){
  read(ftp->cmdFD, cmdBuff, CMD_BUFF_LEN);
  printf("%s\n", cmdBuff);
  
  if((cmdBuff[0] - '0' != POSITIVE_COMP_REPLY) && (cmdBuff[0] - '0' != POSITIVE_INT_REPLY)){
    printf("receiveCmdResponse::Server response error\n");
    exit(2);
  }
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

  printf("Wrote %lu bytes\n", write(ftp->cmdFD, buff, strlen(buff)));
}