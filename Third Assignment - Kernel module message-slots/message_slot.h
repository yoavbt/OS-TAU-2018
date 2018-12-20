#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>

// // Set the message of the device driver
// #define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "message_slot"
#define SUCCESS 0

#define MAJOR_NUM 244

// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#endif
