#include "header.h"

#define STRING                                                                                                                 \
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliq" \
    "ua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aut" \
    "e irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat c" \
    "upidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. vgcvhnvhj vhj vbhjvshjc vjhfb weyu" \
    "ua. Ut enim ad minim veniam, quis nostrud exercitation aut" \
    "upidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. vgcvhnvhj vhj vbhjvshjc vjhfb weyu" \
    "ua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aut" \
    "e irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat c" \
    "upidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. "

typedef struct metadata
{
    time_t sendtime;
    char packet[PACKETSIZE];
    time_t ack_recv_time;
} _data;

_data data[MAXPACKETS] = {0};
int seqmax = 0;

void initialise_data()
{
    for (int i = 0; i < MAXPACKETS; i++)
    {
        bzero(data[i].packet, PACKETSIZE);
        data[i].sendtime = 0;
        data[i].ack_recv_time = 0;
    }
}

void print_pkt_sent()
{
    for (int i = 0; i < seqmax; i++)
    {
        if (data[i].sendtime)
            printf(ANSI_FG_COLOR_BLUE "#");
        else
            printf(ANSI_FG_COLOR_MAGENTA "-");
    }
    printf(ANSI_COLOR_RESET "\n");
}

void print_ack_recd()
{
    for (int i = 0; i < seqmax; i++)
    {
        if (data[i].ack_recv_time)
            printf(ANSI_FG_COLOR_GREEN "0");
        else
            printf(ANSI_FG_COLOR_YELLOW "O");
    }
    printf(ANSI_COLOR_RESET "\n");
}

void send_custom(int sockfd, int ind, struct sockaddr_in *addr, socklen_t addr_size)
{
    sendto(sockfd, data[ind].packet, PACKETSIZE, 0, (struct sockaddr *)addr, addr_size);
    data[ind].sendtime = time(NULL);
}

void receive_ack(int sockfd, char *buff, struct sockaddr_in *addr, socklen_t *addr_size)
{
    int seqnum = -1;
    int len = 1;
    bzero(buff, PACKETSIZE);
    while (len > 0)
    {
        len = recvfrom(sockfd, buff, PACKETSIZE, 0, (struct sockaddr *)addr, addr_size);
        if (len > 0 && !strncmp(buff, "ack", 3))
        {
#ifdef DEBUG
            for (int j = 0; j < PACKETSIZE; j++)
                printf("%c", !buff[j] ? ' ' : buff[j]);
            printf("\n");
#endif
            sscanf(buff + DATASIZE, "%08d", &seqnum);
            if (seqnum < 0 || seqnum >= MAXPACKETS)
            {
                printf("%d - Invalid sequence number!\n", seqnum);
                exit(EXIT_FAILURE);
            }
            if (data[seqnum].ack_recv_time)
                break;
            data[seqnum].ack_recv_time = time(NULL);
            printf("ack%02d ", seqnum);
            print_ack_recd();
        }
    }
}

int is_all_recd()
{
    for (int i = 0; i < seqmax; i++)
        if (!data[i].ack_recv_time)
            return 0;
    return 1;
}

int min_ind_to_send()
{
    for (int i = 0; i < seqmax; i++)
        if (!data[i].ack_recv_time)
            return i;
    return 0;
}

int main()
{
    char *ip = "127.0.0.1";
    int port = 5600;
    int len, sockfd;
    char buffer[PACKETSIZE];
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size = sizeof(client_addr), server_addr_size = sizeof(server_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    printerror(sockfd, "Socket error");
    printf("[+]UDP server socket created.\n");

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    printerror(bind(sockfd, (struct sockaddr *)&server_addr, server_addr_size), "Bind error");
    printf("[+]Binded to the port number: %d\n", port);
    printerror(recvfrom(sockfd, buffer, PACKETSIZE, 0, (struct sockaddr *)&client_addr, &client_addr_size), "Connection request");
    printf(ANSI_FG_COLOR_GREEN "Connection established\n" ANSI_COLOR_RESET);

    initialise_data();

    char *string = STRING;
    int str_len = strlen(string);
    printf("Strlen %d\n", str_len);

    // break data into packets
    int num_bytes_left = str_len;
    int ind = 0;
    while (num_bytes_left)
    {
        int avl_bytes = min(DATASIZE, num_bytes_left);
        strncpy(data[ind].packet, string, avl_bytes);
        sprintf((data[ind].packet + DATASIZE), "%08d", ind);
        string += DATASIZE;
        num_bytes_left -= avl_bytes;
        ind++;
    }

#ifdef DEBUG
    for (int i = 0; i < ind; i++)
    {
        for (int j = 0; j < PACKETSIZE; j++)
            printf("%c", !data[i].packet[j] ? ' ' : data[i].packet[j]);
        printf("\n");
    }
#endif

    // send seqmax
    seqmax = ind;
    printf(ANSI_FG_COLOR_BLUE "Seqmax is " ANSI_FG_COLOR_MAGENTA "%d\n" ANSI_COLOR_RESET, seqmax);
    bzero(buffer, sizeof(buffer));
    sprintf(buffer, "seqmax%d", seqmax);
    sendto(sockfd, buffer, PACKETSIZE, 0, (struct sockaddr *)&client_addr, client_addr_size);
    bzero(buffer, PACKETSIZE);

    int flags = fcntl(sockfd, F_GETFL, 0);
    printerror(flags, "Error getting socket flags");
    printerror((fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1), "Error setting non-blocking mode");

    ind = 0;
    time_t srt = time(NULL), end;
    while (!is_all_recd(seqmax))
    {
        if (ind > -1 && ind < seqmax && !data[ind].ack_recv_time)
        {
            if (data[ind].sendtime)
            {
                if (time(NULL) - data[ind].sendtime > TIME_WAIT)
                    goto send_data;
            }
            else
            {
            send_data:
                send_custom(sockfd, ind, &client_addr, client_addr_size);
                printf("pkt%02d ", ind);
                print_pkt_sent();
                sleep(TIME_BETWEEN_PACKETS);
            }
        }
        else
            ind = min_ind_to_send() - 1;

        receive_ack(sockfd, buffer, &client_addr, &client_addr_size);
        ind += 1;
    }
    end = time(NULL);

    printf(ANSI_FG_COLOR_BLUE "\nTotal bytes received is " ANSI_FG_COLOR_MAGENTA "%d\n" ANSI_FG_COLOR_BLUE "Total time taken for transmission is " ANSI_FG_COLOR_MAGENTA "%ds\n" ANSI_COLOR_RESET,
           str_len, end - srt);
    for (int i = 0; i < seqmax; i++)
        printf("Sent pkt%d in %ds\n", i, data[i].ack_recv_time - data[i].sendtime);
}