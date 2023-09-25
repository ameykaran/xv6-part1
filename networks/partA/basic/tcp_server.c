#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    char *ip = "127.0.0.1";
    int port = 5500;

    int server_sock, client1_sock, client2_sock;
    struct sockaddr_in server_addr, client1_addr, client2_addr;
    socklen_t addr_size;
    char buffer[1024];
    int n;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP server socket created.\n");

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = port;
    server_addr.sin_addr.s_addr = inet_addr(ip);

    n = bind(server_sock, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (n < 0) {
        perror("[-]Bind error");
        exit(1);
    }
    printf("[+]Binded to the port number: %d\n", port);

    listen(server_sock, 5);
    printf("Listening...\n");

    while (1) {
        addr_size = sizeof(client1_addr);
        client1_sock = accept(server_sock, (struct sockaddr *) &client1_addr, &addr_size);
        printf("[+]Client 1 connected.\n");
        client2_sock = accept(server_sock, (struct sockaddr *) &client2_addr, &addr_size);
        printf("[+]Client 2 connected.\n");

        bzero(buffer, 1024);
        recv(client1_sock, buffer, sizeof(buffer), 0);
        printf("Client: %s\n", buffer);

        bzero(buffer, 1024);
        strcpy(buffer, "HI, THIS IS SERVER. HAVE A NICE DAY!!!");
        printf("Server: %s\n", buffer);
        send(client1_sock, buffer, strlen(buffer), 0);


    }

    return 0;
}