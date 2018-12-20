#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>  
#include <linux/module.h>  
#include <linux/fs.h>      /* for register_chrdev */
#include <linux/uaccess.h> /* for get_user and put_user */
#include <linux/string.h>  /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>    /* for kmalloc and kfree*/
#include "message_slot.h"

MODULE_LICENSE("GPL");

/**
 * The data structure of the module is as followed:
 * 1. The main structure (module_data) is a linked list representing all the different message_slots
 *    that the user had created.
 * inside module_data there is another linked list structure called:
 * 2. message_slot_channel_list that represent the data structure for each channel 
 *    inside a specific message_slots.
 **/

// The linked list structure for different message_slots
struct module_data
{
  int minor; // the minor number of the message slot
  struct module_data *next; // the next message slot item
  struct message_slot_channel_list *start; // the channels linked list for this message slot
};
static struct module_data *start_module = NULL; // The first element in the module data linked list
static struct module_data *curr_module = NULL; // The current message slot we are working on

// The linked list structure for different channels id inside a specific channel slot
struct message_slot_channel_list
{
  int channel_number; // channel id
  struct message_slot_channel_list *next; // the next channel item
  char buffer[128]; // the buffer
  int sizeOfMessage; // the size of the message
};

//================== Utility FUNCTIONS ===========================


// Deleting all memory of the channel id linked list
static void free_all(struct message_slot_channel_list *node)
{
  while (node)
  {
    free_all(node->next);
    kfree(node);
    break;
  }
}
// Deleting all memory of the message slot linked list
void free_all_module(struct module_data *node)
{
  while(node)
  {
    free_all_module(node->next);
    free_all(node->start);
    kfree(node);
    break;
  }
}

//Find a specific channel id node inside a message slot 
static struct message_slot_channel_list
    *
    find_node(struct message_slot_channel_list *node, int channel)
{
  while (node != NULL && node->channel_number != channel)
  {
    return find_node(node->next, channel);
  }
  return node;
}

//================== DEVICE FUNCTIONS ===========================


// The open function add a new node the the message slot linked list if the node does not exist
// and update the curr_module node to be the curr message slot node in the linked list
static int device_open(struct inode *inode,
                       struct file *file)
{
  printk("Invoking device_open(%p)\n", file);
  int minor;
  minor = (int)iminor(inode);
  // If this is the first message slot node in the message slot linked list
  if (start_module == NULL)
  {
    struct module_data *new_node = (struct module_data *)
        kmalloc(sizeof(struct module_data), GFP_KERNEL);
    if (new_node == NULL)
    {
      printk("Error in kernel memory allocation\n");
      return -EINVAL;
    }
    new_node->minor = minor;
    new_node->start = NULL;
    new_node->next = NULL;
    start_module = new_node;
    curr_module = start_module;
  }
  else
  {
    int i;
    i = 0;
    struct module_data *temp = start_module;
    // Finding the correct message slot node based on the minor number
    while (temp != NULL)
    {
      if (temp->minor == minor)
      {
        i = 1;
        curr_module = temp;
        break;
      }
      temp = temp->next;
    }
    // If a message slot with the given minor does not exist in the list - create a new node and add
    // it to the list
    if (i == 0)
    {
      struct module_data *new_node = (struct module_data *)
          kmalloc(sizeof(struct module_data), GFP_KERNEL);
      if (new_node == NULL)
      {
        printk("Error in kernel memory allocation\n");
        return -EINVAL;
      }
      new_node->minor = minor;
      new_node->start = NULL;
      curr_module = new_node;
      new_node->next = start_module->next;
      start_module->next = new_node;
    }
  }
  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release(struct inode *inode,
                          struct file *file)
{
  printk("Invoking device_release(%p,%p)\n", inode, file);

  return SUCCESS;
}

//---------------------------------------------------------------
// The Read function try to read from the channel id node of the curr_module message node
static ssize_t device_read(struct file *file,
                           char __user *buffer,
                           size_t length,
                           loff_t *offset)
{

  printk("Invoking device_read(%p,%d)\n", file, length);
  if (!(file->private_data)) // if no channel had been provided
  {
    printk("No channel had been set\n");
    return -EINVAL;
  }
  int ch = (int)file->private_data;
  struct message_slot_channel_list *curr = find_node(curr_module->start, ch); // if no message is found on the channel
  if (!curr)
  {
    printk("No message exist in this channel\n");
    return -EWOULDBLOCK;
  }
  if ( (curr -> sizeOfMessage) > length) // if the size of the buffer is smaller than 128 bytes
  {
    printk( "The Provided buffer is to small for conataining the message. buffer size is: %d and message size if: %d\n",length,(curr -> sizeOfMessage));
    return -ENOSPC;
  }
  int i;
  for (i = 0; i < (curr -> sizeOfMessage) ; ++i)
  {
    // Creating atomic operation - if one writing/reading byte failed : return einval
    if(put_user((curr->buffer)[i], &buffer[i]) ==  -EFAULT){
      return -EINVAL;
    };
  }
  return (curr -> sizeOfMessage);
}

//---------------------------------------------------------------
// The write function update the buffer inside the channel id node.
// If such node not found than it creates a new channel id node in the curr_module message slot node 
static ssize_t device_write(struct file *file,
                            const char __user *buffer,
                            size_t length,
                            loff_t *offset)
{

  printk("Invoking device_write(%p,%d)\n", file, length);
  if (!(file->private_data)) // if channel id not set properly
  {
    printk("No channel had been set\n");
    return -EINVAL;
  }
  if (length == 0 || length > BUF_LEN) // if the size of the message is 0 or bigger than 128 bytes
  {
    printk("The size of the message been passed is greater than 128 Bytes or Empty\n");
    return -EMSGSIZE;
  }
  int i;
  int ch = (int)file->private_data;
  struct message_slot_channel_list *curr;
  if (curr_module->start == NULL) // If the channel id list is empty
  {
    struct message_slot_channel_list *new_node = (struct message_slot_channel_list *)
        kmalloc(sizeof(struct message_slot_channel_list), GFP_KERNEL);
    if (new_node == NULL)
    {
      printk("Error in kernel memory allocation\n");
      return -EINVAL;
    }
    new_node->channel_number = ch;
    new_node -> sizeOfMessage = length + 1;
    curr_module->start = new_node;
    curr = new_node;
    new_node->next = NULL;
  }
  else
  {
    curr = find_node(curr_module->start, ch);
    if (curr == NULL) // If the channel id node does not found in the channel id list
    {
      struct message_slot_channel_list *new_node = (struct message_slot_channel_list *)
          kmalloc(sizeof(struct message_slot_channel_list), GFP_KERNEL);
      if (new_node == NULL)
      {
        printk("Error in kernel memory allocation\n");
        return -EINVAL;
      }
      new_node->channel_number = ch;
      new_node -> sizeOfMessage = length + 1;
      new_node->next = (curr_module->start)->next;
      (curr_module->start)->next = new_node;
      curr = new_node;
    }
    // The curr node is representing the correct channel id node
    curr -> sizeOfMessage = length + 1;
  }

  for (i = 0; i < length ; ++i)
  {
    // Creating atomic operation - if one writing/reading byte failed : return einval
    if(get_user((curr->buffer)[i], &buffer[i]) == -EFAULT){
      return -EINVAL;
    };
  }
  // The end of the message string 
  curr->buffer[length] = '\0';
  return length;
}

//----------------------------------------------------------------
static long device_ioct(struct file *file,
                        unsigned int ioctl_command_id,
                        unsigned long ioctl_param)
{
  // Switch according to the ioctl called
  if (MSG_SLOT_CHANNEL == ioctl_command_id)
  {
    // Get the parameter given to ioctl by the process
    printk("Invoking ioctl: setting channel to %ld\n",
           ioctl_param);
    if (ioctl_param <= 0)
    {
      return -EINVAL;
    }
    // set the file -> privat_date to be the channel id
    file->private_data = (void *)ioctl_param;
    return SUCCESS;
  }
  return -EINVAL;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
    {
        .read = device_read,
        .write = device_write,
        .open = device_open,
        .unlocked_ioctl = device_ioct,
        .release = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
  memset(&start_module, 0, sizeof(struct message_slot_channel_list *));
  memset(&curr_module, 0, sizeof(struct message_slot_channel_list *));
  int major;
  major = register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);
  // Negative values signify an error
  if (major < 0)
  {
    printk(KERN_ERR "%s registraion failed\n",
           DEVICE_FILE_NAME);
    return major;
  }
  printk(KERN_INFO "message_slot: registered major number %d\n", MAJOR_NUM);

  return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
  printk(KERN_INFO "message_slot: UNREGISTER major number %d\n", MAJOR_NUM);
  // free all memory
  free_all_module(start_module);
  // Unregister the device
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
