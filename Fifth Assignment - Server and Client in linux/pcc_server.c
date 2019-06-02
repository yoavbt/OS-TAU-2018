#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <ctype.h>
#include <signal.h>

int print_char[95] = {0}; // Data structure for printable characters
int process = 1; // Flag for SIGINT
int total_chars_to_read = 0; // How many chars will the server recieve from the client
// Print all printable character function
void print_all_chars(int *print_char, int size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        printf("char '%c' : %u times\n", (char)(i + 32), print_char[i]);
    }
}

// The handler for the SIGINT signal
static void hdl(int sig, siginfo_t *siginfo, void *context)
{
    printf("\n");
    process = 0;
}

int main(int argc, char *argv[])
{

    struct sigaction act;

    memset(&act, '\0', sizeof(act));

    act.sa_sigaction = &hdl;

    act.sa_flags = SA_SIGINFO;

    if (sigaction(SIGINT, &act, NULL) < 0)
    {
        fprintf(stderr, "sigaction had failed.\n");
        return 1;
    }

    int listenfd = -1; // socket file descriptor
    int connfd = -1;   // connnection file descriptor

    int total_printable = 0; // Total printable chars

    struct sockaddr_in serv_addr; // The server address info
    socklen_t addrsize = sizeof(struct sockaddr_in);

    listenfd = socket(AF_INET, SOCK_STREAM, 0); // use TCP socket
    if (listenfd < 0)
        {
            printf("\n Error : Socket failed to create. %s \n", strerror(errno));
            return 0;
        }
    memset(&serv_addr, 0, addrsize);

    serv_addr.sin_family = AF_INET;                // Use IPv4 protocol
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY = any local machine address - localhost
    serv_addr.sin_port = htons(atoi(argv[1]));     // Listen to the argv[1] PORT

    if (0 != bind(listenfd,
                  (struct sockaddr *)&serv_addr,
                  addrsize))
    {
        printf("\n Error : Bind Failed. %s \n", strerror(errno));
        return 1;
    }

    if (0 != listen(listenfd, 10)) // Max of 10 clients in the queue
    {
        printf("\n Error : Listen Failed. %s \n", strerror(errno));
        return 1;
    }
    // while SIGINT is not recieved continue handle requests
    while (process)
    {
        // Accept a connection.
        connfd = accept(listenfd,
                        NULL,
                        &addrsize);
        // if there is an error with the connection and SIGINT had been sent
        if (connfd < 0 && !process)
        {
            print_all_chars(print_char, 95);
            return 0;
        }
        if (connfd < 0)
        {
            printf("\n Error : Accept Failed. %s \n", strerror(errno));
            return 1;
        }
        total_printable = 0; // How many printible chars got from the client
        total_chars_to_read = 0; // How many chars will the server recieve from the client
        if (-1 == read(connfd, &total_chars_to_read, sizeof(int)))
        {
            printf("Read from the client had been failed%s\n", strerror(errno));
            return 1;
        };
        char buff_of_chars[total_chars_to_read]; // Create a buffer of chars
        // Read from the client buffer to the server buffer
        while (1)
        {
            int bytes_read = read(connfd,
                                  buff_of_chars,
                                  total_chars_to_read);
            if (bytes_read <= 0)
                break;
        }
        //Check for printalbe chars inside the buffer
        for (int i = 0; i < total_chars_to_read; i++)
        {
            if (isprint((int)buff_of_chars[i]))
            {
                total_printable += 1;
                print_char[(int)buff_of_chars[i] - 32] += 1;
            }
        }
        // Return to the client how many printalble chars it sends
        if (-1 == (write(connfd,
                         &total_printable,
                         sizeof(total_printable))))
            printf("error %s\n", strerror(errno));
        {};
        // close socket
        close(connfd);
    }
    print_all_chars(print_char, 95);
    return 0;
}
