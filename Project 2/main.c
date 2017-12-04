#include "urlParser.h"

int main(int argc, char* argv[]){
  if(argc != 2){
    printf("Expected 1 argument: ftp://[<user>:<password>@]<host>/<url-path>\n");
    exit(1);
  }

  FTP ftp;

  parseUrl(argv[1], &ftp);
  return 0;
}
