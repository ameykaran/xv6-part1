#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define printerror(func, msg) \
    if (func < 0)             \
    {                         \
        perror("[-]" msg);    \
        exit(1);              \
    }

int main()
{
    char *ip = "127.0.0.1";
    int port = 5500;

    int sock;
    struct sockaddr_in addr;
    socklen_t addr_size;
    char buffer[1024];
    int n;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    printerror(sock, "Socket error");

    printf("[+]TCP server socket created.\n");

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    printerror(connect(sock, (struct sockaddr *)&addr, sizeof(addr)), "Connection error");
    printf("Connected to the server.\n");

    bzero(buffer, 1024);
    strcpy(buffer, "I am the client.");
    printf("Client: %s\n", buffer);
    printerror(send(sock, buffer, strlen(buffer), 0), "Send to server");

    bzero(buffer, 1024);
    printerror(recv(sock, buffer, sizeof(buffer), 0), "Receiving from server");
    printf("Server: %s\n", buffer);

    close(sock);
    printf("Disconnected from the server.\n");

    return 0;
}