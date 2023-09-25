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

    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    printerror(sockfd, "Socket error");
    printf("[+]UDP server socket created.\n");

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    printerror(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)), "Bind error");

    bzero(buffer, 1024);
    socklen_t addr_size = sizeof(client_addr);
    printerror(recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&client_addr, &addr_size), "Receiving from client");
    printf("[+]Data recv: %s\n", buffer);

    bzero(buffer, 1024);
    strcpy(buffer, "Welcome to the UDP Server.");
    printerror(sendto(sockfd, buffer, 1024, 0, (struct sockaddr *)&client_addr, addr_size), "Sending to the client");
    printf("[+]Data send: %s\n", buffer);

    return 0;
}