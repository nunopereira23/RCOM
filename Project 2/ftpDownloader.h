#ifndef FTP_DOWNLOADER_H
#define FTP_DOWNLOADER_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "urlParser.h"
#define SERVER_CMD_PORT 21
#define POSITIVE_COMP_REPLY 2
#define POSITIVE_INT_REPLY 3
#define POSITIVE_PRE_REPLY 1

#define CMD_BUFF_LEN 1024


int main(int argc, char* argv[]);
void findFileName(char* pathName, char** fileName);
int establishCmdConnection(FTP* ftp);
int establishDataConnection(FTP* ftp, char* ipAddress, int dataPort);

//Command handling functions
void writeCmd(FTP* ftp, char** cmdArgs);
void receiveCmdResponse(FTP* ftp, char* cmdBuff);
void readLine(FTP* ftp, char* buffer);


void receiveFile(FTP* ftp);

//UTILITIES
void addressCat(char* ipFrag1, char* ipFrag2, char* ipFrag3, char* ipFrag4, char* ipAddress);

#endif