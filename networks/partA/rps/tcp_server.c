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

#define receive(sockNum, buff, plrNum)                                        \
    printerror(recv(sockNum, buff, 100, 0), "Receiving from player " plrNum); \
    len = strlen(buff);                                                       \
    if (buff[len - 1] == '\n')                                                \
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

int main()
{
    char *ip = "127.0.0.1";
    int port = 5500, len;

    int server_sock, client1_sock, client2_sock;
    struct sockaddr_in server_addr, client1_addr, client2_addr;
    socklen_t addr1_size = sizeof(client1_addr);
    socklen_t addr2_size = sizeof(client2_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    printerror(server_sock, "Socket error");
    printf("[+]TCP server socket created.\n");

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client1_addr, 0, addr1_size);
    memset(&client2_addr, 0, addr2_size);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    printerror(bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)),
               "Bind error");
    printf("[+]Binded to the port number: %d\n", port);

    printerror(listen(server_sock, 2), "Listen");
    printf("Waiting for players...\n");

    char choice1[100] = {0}, choice2[100] = {0};
    char *init = "Welcome to rock, paper, scissors game!\nEnter your name";
    char *endMsg = "Do you want to play again? [y|n]";

    client1_sock = accept(server_sock, (struct sockaddr *)&client1_addr, &addr1_size);
    printerror(client1_sock, "Accept");
    printerror(send(client1_sock, init, strlen(init), 0), "Sending to player 1");
    receive(client1_sock, choice1, "1");
    printf("[+]Player 1 connected: %s\n", choice1);

    client2_sock = accept(server_sock, (struct sockaddr *)&client2_addr, &addr2_size);
    printerror(client2_sock, "Accept");
    printerror(send(client2_sock, init, strlen(init), 0), "Sending to player 2");
    receive(client2_sock, choice2, "2");
    printf("[+]Player 2 connected: %s\n", choice2);

    char *pl1_name = strdup(choice1);
    char *pl2_name = strdup(choice2);

    printf("\n");
    while (1)
    {
        bzero(choice1, 100);
        bzero(choice2, 100);

        receive(client1_sock, choice1, "1");
        receive(client2_sock, choice2, "2");

        printf("%s played %s; %s played %s\n", pl1_name, choice1, pl2_name, choice2);

        // Judgement
        char result[10] = {0};
        getOutcome(choice1, choice2, result);

        printerror(send(client1_sock, result, strlen(result), 0), "Sending judgement to player 1");
        printerror(send(client2_sock, result, strlen(result), 0), "Sending judgement to player 2");

        // printf("Sent judgement |%s|\n", result);

        printerror(recv(client1_sock, choice1, 100, 0), "Dummy - player 1");
        printerror(recv(client2_sock, choice2, 100, 0), "Dummy - player 2");
        if (result[0] == '0')
            printf("It's a tie\n");
        else if (result[0] == '1')
            printf("%s won the game\n", pl1_name);
        else if (result[0] == '2')
            printf("%s won the game\n", pl2_name);

        printerror(send(client1_sock, endMsg, strlen(endMsg), 0), "Sending ending message to player 1");
        printerror(send(client2_sock, endMsg, strlen(endMsg), 0), "Sending ending message to player 2");

        bzero(choice1, 100);
        bzero(choice2, 100);

        printerror(recv(client1_sock, choice1, 100, 0), "Receiving confirmation from player 1");
        printerror(recv(client2_sock, choice2, 100, 0), "Receiving confirmation from player 2");

        char buff[100] = {0};
        strcpy(buff, "null");
        if (!(choice1[0] == 'y' && choice2[0] == 'y'))
            strcpy(buff, "exit");

        printerror(send(client1_sock, buff, strlen(buff), 0), "Sending sigterm to player 1");
        printerror(send(client2_sock, buff, strlen(buff), 0), "Sending sigterm to player 2");

        if (!(choice1[0] == 'y' && choice2[0] == 'y'))
            break;

        printf("\n");
    }
    close(server_sock);
    close(client1_sock);
    close(client2_sock);
    printf("[-]%s disconnected.\n", pl1_name);
    printf("[-]%s disconnected.\n", pl2_name);
    printf("Game ended...\n");
    return 0;
}