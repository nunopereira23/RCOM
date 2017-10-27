#include "appLayer.h"
#include "linkLayer.h"


int test(void){

  if(linkLayer.prog == TRANSMISSOR){
      linkLayer.seqNum = 0;
    unsigned char packect[] = "Ola tudo bem?";

    printf("llwrite wrote %d\n", llwrite(linkLayer.fd, packect, 14));
  }
  else{
    linkLayer.seqNum = 1;
    printf("llread read %d\n", llread(linkLayer.fd, linkLayer.frame));
    int i;
    for(i = 0; i < linkLayer.frameSize; i++){
      printf("Hex %x\n", linkLayer.frame[i]);
    }
    printf("Dados %s\n", linkLayer.frame);
  }
  return 0;
}
