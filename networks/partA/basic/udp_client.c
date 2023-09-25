#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define printerror(func, msg) \
    if (func < 0)             \
    {                         \
        perror("[-]" msg);    \
        exit(1);              \
    }

int main(int argc, char **argv)
{
    char *ip = "127.0.0.1";
    int port = 5600;

    struct sockaddr_in addr;
    char buffer[1024];

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    printerror(sockfd, "Socket error");
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    bzero(buffer, 1024);
    strcpy(buffer, "I am the client.");
    printerror(sendto(sockfd, buffer, 1024, 0, (struct sockaddr *)&addr, sizeof(addr)), "Sending to the client");
    printf("[+]Data send: %s\n", buffer);

    bzero(buffer, 1024);
    socklen_t addr_size = sizeof(addr);
    printerror(recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&addr, &addr_size), "Receiving from client");
    printf("[+]Data recv: %s\n", buffer);

    return 0;
}