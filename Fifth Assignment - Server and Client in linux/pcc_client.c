#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int sockfd = -1; // Socket file descriptor
    unsigned int C = 0;
    unsigned int total_char = atoi(argv[3]);
    char char_buff[total_char];

    struct sockaddr_in serv_addr; // where we Want to get to

    memset(char_buff, 0, sizeof(char_buff));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // use TCP socket
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; // // Use IPv4 protocol
    serv_addr.sin_port = htons(atoi(argv[2])); // Connect to the Port at the second argument
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]); // Connect to the IP at the first argument      
    if (connect(sockfd,
                (struct sockaddr *)&serv_addr,
                sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed. %s \n", strerror(errno));
        return 1;
    }
  
    // int notwritten = 3;
    int nsent = write(sockfd,
                      &total_char,
                      sizeof(total_char));
    if (-1 == nsent)
    {
        close(sockfd);
        printf("error in client %s\n", strerror(errno));
        return 1;
    }
    unsigned int t = 0;
    int urnd = open("/dev/urandom", O_RDONLY);
    while (1)
    {
        t = read(urnd, &char_buff, total_char);
        if (t == total_char)
        {
            break;
        }
        if (-1 == t)
        {
            close(sockfd);
            printf("error in client %s\n", strerror(errno));
            return 1;
        }
    }
    close(urnd);
    nsent = write(sockfd,
                  char_buff,
                  sizeof(char_buff));
    if (-1 == nsent)
    {
        close(sockfd);
        printf("error in client %s\n", strerror(errno));
        return 1;
    }
    // waiting with the connection so the server can write
    shutdown(sockfd, SHUT_WR);
    int i = read(sockfd,
                 &C,
                 sizeof(C));
    if (-1 == i)
    {
        printf("error %s\n", strerror(errno));
        close(sockfd);
        return 1;
    }
    close(sockfd); 
    printf("# of printable characters: %u\n", C);
   
    return 0;
}
