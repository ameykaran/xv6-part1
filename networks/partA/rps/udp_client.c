#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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

int main(int argc, char **argv)
{
    char *ip = "127.0.0.1";
    int port = 5600;

    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);
    char buffer[1024] = {0};

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    printerror(sockfd, "Socket error");
    printf("[+]UDP client socket created.\n");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    printerror(sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, addr_size), "Connect to server");
    printerror(recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addr_size), "Receiving welcome from server");
    printf("%s\n", buffer);

    bzero(buffer, sizeof(buffer));
    read(0, buffer, sizeof(buffer));
    printerror(sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, addr_size), "Send name to server");
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
        printerror(sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, addr_size), "Send choice to server");
        // Judgement
        bzero(buffer, sizeof(buffer));
        printerror(recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addr_size), "Receiving judgement from server");
        if (buffer[0] == '0')
            strcpy(buffer, "It's a tie");
        else if (buffer[0] == playerChar)
            strcpy(buffer, "You won");
        else
            strcpy(buffer, "You lost");
        printf("Outcome: %s\n", buffer);

        printerror(sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, addr_size), "Send dummy to server");

        bzero(buffer, sizeof(buffer));
        printerror(recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addr_size), "Receiving end msg from server");
        writenewline("", 1);
        writenewline(buffer, sizeof(buffer));

        bzero(buffer, sizeof(buffer));
        read(0, buffer, sizeof(buffer));
        printerror(sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, addr_size), "Send continuation to server");
        if (buffer[0] != 'y')
            break;

        bzero(buffer, sizeof(buffer));
        printerror(recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addr_size), "Check for sigkill from server");
        if (!strncmp(buffer, "exit", 4))
            break;
    }

    close(sockfd);
    printf("Disconnected from the server.\n");

    return 0;
}