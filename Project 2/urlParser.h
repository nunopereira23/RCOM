#ifndef URL_PARSER_H
//#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_STRING_LENGHT 100

typedef struct{
  char* username;
  char* password;
  char* path;
  char* fileName;
  struct hostent* h;
  int cmdFD;
  int dataFD;
} FTP;

int parseUrl(char* fullUrl, FTP* ftp);

#endif
