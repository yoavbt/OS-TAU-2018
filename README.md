# TAU Operating System course 18

The following folders contain my solutions for 4 different assignments of the os course 18 at TAU.
The code was written under Linux os and the commands are applying for Linux kernel only.
The programming language is C and VS code was used as the editor.

## The Assignments

### First Assignment - String Formating and  Bash Scripting
Containig two files:
**hw1_subs.c** - the String Formating program itself
**hw1_script.sh** - the shell scripting code to run the program
### Second Assignment - Customize Shell

Containig one file:
**myshell.c** - the customize shell supported the following:
 1. Run noraml Linux command processes
 2. Run background Linux command processes ("&")
 3. Run piped Linux command processes ("|")

### Third Assignment - Kernel module "message-slots" as IPC

Containig five files:
**message_slot.c** - kernel module implementing the message slot IPC mechanism
**message_sender.c** - user space program to send a message
**message_reader.c** - user space program to read a message
**message_slot.h**
**Makefile**

### Fourth Assignment - Distributed search using linux threads

Containig one file:
**distributed_search.c** - multi-threaded folder searching application

### Fifth Assignment - Client and Server in linux using ports and TCP connection

Containig two files:
**pcc_client.c** - linux client send http request to linux server
**pcc_server.c** - linux server send http response to linux client

