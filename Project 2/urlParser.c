#include "urlParser.h"

int parseUrl(char* fullUrl, FTP* ftp){
  if(strncmp(fullUrl, "ftp://", 6)){
    printf("Invalid URL, it must begin with \"ftp://\"\n");
    exit(1);
  }

  char hostname[MAX_STRING_LENGHT];
  char* at = strchr(fullUrl + 6, '@');
  char* pathSlash = strchr(fullUrl + 6, '/');

  if(pathSlash == NULL || *(pathSlash + 1) == '\0'){
    printf("Invalid URL, it must have a /path\n");
    exit(1);
  }

  int userLength = 10;
  int passLength = 5;

  if(at != NULL){
    char* colon = strchr(fullUrl + 6, ':');

    userLength = colon - (fullUrl + 6);
    passLength = at - colon - 1;

    ftp->username = malloc(userLength + 1);
    ftp->password = malloc(passLength + 1);
    strncpy(ftp->username, fullUrl + 6, userLength);
    ftp->username[userLength] = '\0';

    strncpy(ftp->password, colon + 1, passLength);
    ftp->password[passLength] = '\0';

    strncpy(hostname, at + 1, pathSlash - at - 1);
    hostname[pathSlash - at - 1] = '\0';

  }
  else{
    ftp->username = malloc(10);
    ftp->password = malloc(5);
    strcpy(ftp->username, "anonymous");
    strcpy(ftp->password, "pass");

    strncpy(hostname, fullUrl + 6, pathSlash - fullUrl - 6);
    hostname[pathSlash - fullUrl - 6] = '\0';
  }

  ftp->path = pathSlash + 1;

#ifdef DEBUG
  printf("Username: %s - size: %d\n", ftp->username, userLength);
  printf("Password: %s - size: %d\n", ftp->password, passLength);
  printf("Path: %s - size: %lu\n", ftp->path, strlen(ftp->path));
  printf("Hostname: %s - size: %lu\n", hostname, strlen(hostname));
#endif

  if ((ftp->h = gethostbyname(hostname)) == NULL) {
      herror("gethostbyname");
      exit(1);
  }

  #ifdef DEBUG
    printf("parseUrl::Host's Ip: %s\n", inet_ntoa(*((struct in_addr *)ftp->h->h_addr)));
  #endif

  return 0;
}
