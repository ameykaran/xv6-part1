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

#define receive(sockNum, buff, addr, size, plrNum)                                   \
    len = recvfrom(sockNum, buff, sizeof(buff), 0, (struct sockaddr *)&addr, &size); \
    printerror(len, "Receiving from player " plrNum);                                \
    buff[len - 1] = 0;

#define getOutcome(choice1, choice2, result)    \
    if (choice1[0] == 'p' && choice2[0] == 'r') \
        result[0] = '1';                        \
    if (choice1[0] == 'r' && choice2[0] == 'p') \
        result[0] = '2';                        \
    if (choice1[0] == 's' && choice2[0] == 'p') \
        result[0] = '1';                        \
    if (choice1[0] == 'p' && choice2[0] == 's') \
        result[0] = '2';                        \
    if (choice1[0] == 'r' && choice2[0] == 's') \
        result[0] = '1';                        \
    if (choice1[0] == 's' && choice2[0] == 'r') \
        result[0] = '2';                        \
    if (choice1[0] == 'p' && choice2[0] == 'p') \
        result[0] = '0';                        \
    if (choice1[0] == 'r' && choice2[0] == 'r') \
        result[0] = '0';                        \
    if (choice1[0] == 's' && choice2[0] == 's') \
        result[0] = '0';

int main(int argc, char **argv)
{
    char *ip = "127.0.0.1";
    int port = 5600;
    char pl1_name[100] = {0}, pl2_name[100] = {0};
    int len, sockfd;
    struct sockaddr_in server_addr, client1_addr, client2_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    printerror(sockfd, "Socket error");
    printf("[+]UDP server socket created.\n");

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client1_addr, 0, sizeof(client1_addr));
    memset(&client2_addr, 0, sizeof(client2_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    printerror(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)), "Bind error");
    printf("[+]Binded to the port number: %d\n", port);

    char choice1[100] = {0}, choice2[100] = {0};
    char *init = "Welcome to rock, paper, scissors game!\nEnter your name";
    char *endMsg = "Do you want to play again? [y|n]";

    socklen_t addr1_size = sizeof(client1_addr);
    socklen_t addr2_size = sizeof(client2_addr);

    receive(sockfd, pl1_name, client1_addr, addr1_size, "1");
    receive(sockfd, pl2_name, client2_addr, addr2_size, "2");

    printerror(sendto(sockfd, init, strlen(init), 0, (struct sockaddr *)&client1_addr, addr1_size), "Sending init to player 1");
    printerror(sendto(sockfd, init, strlen(init), 0, (struct sockaddr *)&client2_addr, addr2_size), "Sending init to player 2");

    bzero(pl1_name, sizeof(pl1_name));
    bzero(pl2_name, sizeof(pl2_name));
    receive(sockfd, pl1_name, client1_addr, addr1_size, "1");
    printf("[+]Player 1 connected: %s\n", pl1_name);
    receive(sockfd, pl2_name, client2_addr, addr2_size, "1");
    printf("[+]Player 2 connected: %s\n", pl2_name);

    while (1)
    {
        char choice1[2] = {0}, choice2[2] = {0};

        receive(sockfd, choice1, client1_addr, addr1_size, "1");
        receive(sockfd, choice2, client2_addr, addr2_size, "2");

        printf("%s played %s; %s played %s\n", pl1_name, choice1, pl2_name, choice2);

        char result[10] = {0};
        getOutcome(choice1, choice2, result);

        printerror(sendto(sockfd, result, strlen(result), 0, (struct sockaddr *)&client1_addr, addr1_size), "Sending outcome to player 1");
        printerror(sendto(sockfd, result, strlen(result), 0, (struct sockaddr *)&client2_addr, addr2_size), "Sending outcome to player 2");

        printerror(recvfrom(sockfd, choice1, sizeof(choice1), 0, (struct sockaddr *)&client1_addr, &addr1_size), "Dummy 1");
        printerror(recvfrom(sockfd, choice2, sizeof(choice1), 0, (struct sockaddr *)&client2_addr, &addr2_size), "Dummy 2");

        if (result[0] == '0')
            printf("It's a tie\n");
        else if (result[0] == '1')
            printf("%s won the game\n", pl1_name);
        else if (result[0] == '2')
            printf("%s won the game\n", pl2_name);

        printerror(sendto(sockfd, endMsg, strlen(endMsg), 0, (struct sockaddr *)&client1_addr, addr1_size), "Sending ending message to player 1");
        printerror(sendto(sockfd, endMsg, strlen(endMsg), 0, (struct sockaddr *)&client2_addr, addr2_size), "Sending ending message to player 2");

        bzero(choice1, 2);
        bzero(choice2, 2);

        receive(sockfd, choice1, client1_addr, addr1_size, "1");
        receive(sockfd, choice2, client2_addr, addr2_size, "2");

        char buff[100] = {0};
        strcpy(buff, "null");
        if (!(choice1[0] == 'y' && choice2[0] == 'y'))
            strcpy(buff, "exit");

        printerror(sendto(sockfd, endMsg, strlen(endMsg), 0, (struct sockaddr *)&client1_addr, addr1_size), "Sending sigterm to player 1");
        printerror(sendto(sockfd, endMsg, strlen(endMsg), 0, (struct sockaddr *)&client2_addr, addr2_size), "Sending sigterm to player 2");

        if (!(choice1[0] == 'y' && choice2[0] == 'y'))
            break;

        printf("\n");
    }
    close(sockfd);
    printf("[-]%s disconnected.\n", pl1_name);
    printf("[-]%s disconnected.\n", pl2_name);
    printf("Game ended...\n");
    return 0;
}