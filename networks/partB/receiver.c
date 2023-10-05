#include "header.h"

typedef struct metadata
{
    time_t recvtime;
    char packet[PACKETSIZE];
    char ack_sent;
} _data;

_data data[MAXPACKETS] = {0};
int seqmax = 0;

void print_pkt_recd()
{
    for (int i = 0; i < seqmax; i++)
    {
        if (data[i].recvtime)
            printf(ANSI_FG_COLOR_BLUE "#");
        else
            printf(ANSI_FG_COLOR_MAGENTA "-");
    }
    printf(ANSI_COLOR_RESET "\n");
}

void print_ack_sent()
{
    for (int i = 0; i < seqmax; i++)
    {
        if (data[i].ack_sent)
            printf(ANSI_FG_COLOR_GREEN "0");
        else
            printf(ANSI_FG_COLOR_YELLOW "O");
    }
    printf(ANSI_COLOR_RESET "\n");
}

void initialise_data()
{
    for (int i = 0; i < MAXPACKETS; i++)
    {
        bzero(data[i].packet, PACKETSIZE);
        data[i].recvtime = 0;
        data[i].ack_sent = 0;
    }
}

int receive_custom(int sockfd, char *buff, struct sockaddr_in *addr, socklen_t *addr_size)
{
    return recvfrom(sockfd, buff, PACKETSIZE, 0, (struct sockaddr *)addr, addr_size);
}

void send_ack(int sockfd, int ind, struct sockaddr_in *addr, socklen_t addr_size)
{
    char temp[PACKETSIZE];
    bzero(temp, PACKETSIZE);
    sprintf(temp, "ack");
    sprintf(temp + DATASIZE, "%08d", ind);
    sendto(sockfd, temp, PACKETSIZE, 0, (struct sockaddr *)addr, addr_size);
    data[ind].ack_sent = 1;
}

int is_all_recd()
{
    for (int i = 0; i < seqmax; i++)
        if (!data[i].ack_sent)
            return 0;
    return 1;
}

int main()
{
    char *ip = "127.0.0.1";
    int port = 5600;
    int len, sockfd;
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(addr);
    char buffer[PACKETSIZE] = {0};

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    printerror(sockfd, "Socket error");
    printf("[+]UDP server socket created.\n");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    buffer[0] = 2;
    printerror(sendto(sockfd, buffer, PACKETSIZE, 0, (struct sockaddr *)&addr, addr_size), "Connection request");
    printf(ANSI_FG_COLOR_GREEN "Connection established\n" ANSI_COLOR_RESET);

    initialise_data();

    // obtain seqmax
    receive_custom(sockfd, buffer, &addr, &addr_size);
    sscanf(buffer, "seqmax%d", &seqmax);
    printf(ANSI_FG_COLOR_BLUE "Seqmax is " ANSI_FG_COLOR_MAGENTA "%d\n" ANSI_COLOR_RESET, seqmax);
    bzero(buffer, PACKETSIZE);

    int flags = fcntl(sockfd, F_GETFL, 0);
    printerror(flags, "Error getting socket flags");
    printerror((fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1), "Error setting non-blocking mode");

    time_t srt = time(NULL), end;
    int timeout_ctr = 0;
    while (1)
    {
        if (is_all_recd())
            break;

        bzero(buffer, PACKETSIZE);
        if (receive_custom(sockfd, buffer, &addr, &addr_size) <= 0)
        {
            if (timeout_ctr > TIME_CLOSE)
            {
                printf(ANSI_FG_COLOR_MAGENTA "Connection timeout\n" ANSI_COLOR_RESET);
                exit(EXIT_FAILURE);
            };
            timeout_ctr += 1;
        }
        else
            timeout_ctr = 0;
#ifdef DEBUG
        for (int j = 0; j < PACKETSIZE; j++)
            printf("%c", !buffer[j] ? ' ' : buffer[j]);
        printf("\n");
#endif
        if (!strncmp(buffer, "ack", 3))
            continue;

        int seqnum;
        sscanf(buffer + DATASIZE, "%08d", &seqnum);
        if (seqnum < 0 || seqnum >= MAXPACKETS)
        {
            printf("Invalid sequence number!\n");
            continue; // exit(EXIT_FAILURE);
        }
        data[seqnum].recvtime = time(NULL);
        printf("pkt%d "), seqnum;
        print_pkt_recd();

        buffer[120] = 0;
        strcpy(data[seqnum].packet, buffer);
        sleep(TIME_BETWEEN_PACKETS);
        send_ack(sockfd, seqnum, &addr, addr_size);
        data[seqnum].ack_sent = 1;
        printf("ack%02d ", seqnum);
        print_ack_sent();

        sleep(TIME_BETWEEN_PACKETS);
    }

    end = time(NULL);

    // stitch all packets together
    char str[seqmax * DATASIZE];
    for (int i = 0; i < seqmax; i++)
        strncpy((str + i * DATASIZE), data[i].packet, DATASIZE);

    printf("\n"
           "Data received is\n"
           "****************\n");
    printf("%s\n\n", str);
    printf(ANSI_FG_COLOR_BLUE "Total bytes received is " ANSI_FG_COLOR_MAGENTA "%d\n" ANSI_FG_COLOR_BLUE "Total time taken for transmission is " ANSI_FG_COLOR_MAGENTA "%ds\n" ANSI_COLOR_RESET,
           strlen(str), end - srt);
}