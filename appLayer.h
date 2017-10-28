#ifndef APP_LAYER_H
#define APP_LAYER_H

int test(void);

typedef struct{
  int fileFD;
  unsigned char* packet;
  unsigned int packetSize;
} AppLayer;

#endif
