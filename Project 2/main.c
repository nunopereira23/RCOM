#include "urlParser.h"

void findFileName(char* pathName, char** fileName);

int main(int argc, char* argv[]){
  if(argc != 2){
    printf("Expected 1 argument: ftp://[<user>:<password>@]<host>/<url-path>\n");
    exit(1);
  }

  FTP ftp;

  parseUrl(argv[1], &ftp);
  findFileName(ftp.path, &ftp.fileName);

  #ifdef DEBUG
    printf("FileName: %s\n", ftp.fileName);
  #endif



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
