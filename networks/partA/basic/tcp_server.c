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

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[1024];
    int n;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    printerror(server_sock, "Socket error");
    printf("[+]TCP server socket created.\n");

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    n = bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printerror(n, "Bind error");
    printf("[+]Binded to the port number: %d\n", port);

    printerror(listen(server_sock, 5), "Listen");
    printf("Listening...\n");

    addr_size = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
    printerror(client_sock, "Accept");
    printf("[+]Client connected.\n");

    bzero(buffer, 1024);
    printerror(recv(client_sock, buffer, sizeof(buffer), 0), "Receiving from the client");
    printf("Client: %s\n", buffer);

    bzero(buffer, 1024);
    strcpy(buffer, "Hello from the server!");
    printf("Server: %s\n", buffer);
    printerror(send(client_sock, buffer, strlen(buffer), 0), "Sending to the client");

    close(client_sock);
    printf("[+]Client disconnected.\n\n");

    return 0;
}