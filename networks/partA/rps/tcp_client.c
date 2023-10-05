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

char newline[2] = "\n";
#define writenewline(text, size) \
    write(1, text, size);        \
    write(1, newline, 2);

#ifdef PLAYER1
#define playerChar '1'
#elif PLAYER2
#define playerChar '2'
#endif

int main()
{
    char *ip = "127.0.0.1";
    int port = 5500;

    int sock;
    struct sockaddr_in addr;
    socklen_t addr_size;
    char buffer[1024] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    printerror(sock, "Socket error");
    printf("[+]TCP client socket created.\n");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    printerror(connect(sock, (struct sockaddr *)&addr, sizeof(addr)), "Connection error");
    printf("Connected to the server.\n");

    bzero(buffer, sizeof(buffer));
    printerror(recv(sock, buffer, sizeof(buffer), 0), "Receiving welcome from server");
    printf("%s\n", buffer);

    bzero(buffer, sizeof(buffer));
    read(0, buffer, sizeof(buffer));
    printerror(send(sock, buffer, strlen(buffer), 0), "Send name to server");
    printf("Glad to have you %s\n", buffer);

    while (1)
    {
        write(1, "Enter your choice [r|p|s]: ", 28);
        bzero(buffer, sizeof(buffer));
        read(0, buffer, sizeof(buffer));
        while (buffer[0] != 'r' && buffer[0] != 'p' && buffer[0] != 's')
        {
            write(1, "Please enter a valid choice!\n    [r|p|s]:", 42);
            read(0, buffer, sizeof(buffer));
        }
        printerror(send(sock, buffer, strlen(buffer), 0), "Send choice to server");

        // Judgement
        bzero(buffer, sizeof(buffer));
        printerror(recv(sock, buffer, sizeof(buffer), 0), "Receiving judgement from server");
        if (buffer[0] == '0')
            strcpy(buffer, "It's a tie");
        else if (buffer[0] == playerChar)
            strcpy(buffer, "You won");
        else
            strcpy(buffer, "You lost");
        printf("Outcome: %s\n", buffer);

        printerror(send(sock, buffer, strlen(buffer), 0), "Send dummy to server");

        bzero(buffer, sizeof(buffer));
        printerror(recv(sock, buffer, sizeof(buffer), 0), "Receiving end msg from server");
        writenewline("", 1);
        writenewline(buffer, sizeof(buffer));

        bzero(buffer, sizeof(buffer));
        read(0, buffer, sizeof(buffer));
        printerror(send(sock, buffer, strlen(buffer), 0), "Send continuation to server");
        if (buffer[0] != 'y')
            break;

        bzero(buffer, sizeof(buffer));
        printerror(recv(sock, buffer, sizeof(buffer), 0), "Check for sigkill from server");
        if (!strncmp(buffer, "exit", 4))
            break;
    }

    close(sock);
    printf("Disconnected from the server.\n");

    return 0;
}