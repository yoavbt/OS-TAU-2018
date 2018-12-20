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
  char message[BUF_LEN];
  // OPEN the message slot at path argv[1]
  file_desc = open(argv[1], O_RDWR);
  if (file_desc < 0)
  {
    printf("Can't open device file\n");
    return 1;
  }
  // SET the channel id for the message slot
  if (ioctl(file_desc, MSG_SLOT_CHANNEL, atoi(argv[2]))< 0){
    printf("Error in setting the message channel \n");
    return 1;
  };
  // READ from the messgae slot and from the channel id the message to the message buffer
  if (read(file_desc, message, BUF_LEN) < 0)
  {
    printf("Error in Reading from the message slot\n");
    return 1;
  }
  // Print the message to the sdtout
  printf("Reading successful.. the message is: \n%s\n", message);
  // Closing the message slot
  close(file_desc);
  return 0;
}
