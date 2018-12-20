#include <fcntl.h>     /* open */
#include <unistd.h>    /* exit */
#include <sys/ioctl.h> /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "message_slot.h"

int main(int argc, char *argv[])
{

  int file_desc;
  // OPEN the message slot at path argv[1]
  file_desc = open(argv[1], O_RDWR);
  if (file_desc < 0)
  {
    printf("Can't open device file\n");
    exit(-1);
  }
  // SET the channel id for the message slot
  if (ioctl(file_desc, MSG_SLOT_CHANNEL, atoi(argv[2]))< 0){
    printf("Error in setting the message channel \n");
    return 1;
  };
  // WRITE the message from argv[3] to the buffer
  if (write(file_desc, argv[3], strlen(argv[3])) < 0)
  {
    printf("Error in Writing to the message slot\n");
    return 1;
  }
  // Closing the message slot
  close(file_desc);
  printf("Writing successful\n");

  return 0;
}
