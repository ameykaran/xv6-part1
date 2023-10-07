#include "header.h"

#define STRING                                                                                                                 \
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliq" \
    "ua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aut" \
    "e irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat c" \
    "upidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum. vgcvhnvhj vhj vbhjvshjc vjhfb weyu" \
    "ua. Ut enim ad minim veniam, quis nostrud exercitation aut"

typedef struct metadata
{
    time_t sendtime, recvtime;
    char packet[PACKETSIZE];
    time_t ack_recv_time;
    char ack_sent;
} _data;

_data data[MAXPACKETS] = {0};
int seqmax = 0;

char *ip = "127.0.0.1";
int sockfd;
struct sockaddr_in server_addr;
socklen_t server_addr_size;
int port = PORT;

void initialise_data()
{
    for (int i = 0; i < MAXPACKETS; i++)
    {
        bzero(data[i].packet, PACKETSIZE);
        data[i].sendtime = 0;
        data[i].recvtime = 0;
        data[i].ack_recv_time = 0;
        data[i].ack_sent = 0;
    }
}

void server_print_pkt_sent()
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

void server_print_ack_recd()
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

void client_print_pkt_recd()
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

void client_print_ack_sent()
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

void server_send_custom(int sockfd, int ind, struct sockaddr_in *addr, socklen_t addr_size)
{
    sendto(sockfd, data[ind].packet, PACKETSIZE, 0, (struct sockaddr *)addr, addr_size);
    data[ind].sendtime = time(NULL);
}

void server_receive_ack(int sockfd, char *buff, struct sockaddr_in *addr, socklen_t *addr_size)
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
            server_print_ack_recd();
        }
    }
}

int client_receive_custom(int sockfd, char *buff, struct sockaddr_in *addr, socklen_t *addr_size)
{
    return recvfrom(sockfd, buff, PACKETSIZE, 0, (struct sockaddr *)addr, addr_size);
}

void client_send_ack(int sockfd, int ind, struct sockaddr_in *addr, socklen_t addr_size)
{
    char temp[PACKETSIZE];
    bzero(temp, PACKETSIZE);
    sprintf(temp, "ack");
    sprintf(temp + DATASIZE, "%08d", ind);
    sendto(sockfd, temp, PACKETSIZE, 0, (struct sockaddr *)addr, addr_size);
    data[ind].ack_sent = 1;
}

int server_is_all_recd()
{
    for (int i = 0; i < seqmax; i++)
        if (!data[i].ack_recv_time)
            return 0;
    return 1;
}

int client_is_all_sent()
{
    for (int i = 0; i < seqmax; i++)
        if (!data[i].ack_sent)
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

void server_code()
{
    server_addr_size = sizeof(server_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    printerror(sockfd, "Socket error");
    printf("[+]UDP server socket created.\n");
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    char buffer[PACKETSIZE] = {0};
    struct sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    memset(&client_addr, 0, sizeof(client_addr));

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
    while (!server_is_all_recd(seqmax))
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
                server_send_custom(sockfd, ind, &client_addr, client_addr_size);
                printf("pkt%02d ", ind);
                server_print_pkt_sent();
                sleep(TIME_BETWEEN_PACKETS);
            }
        }
        else
            ind = min_ind_to_send() - 1;

        server_receive_ack(sockfd, buffer, &client_addr, &client_addr_size);
        ind += 1;
    }
    end = time(NULL);

    printf(ANSI_FG_COLOR_BLUE "\nTotal bytes received is " ANSI_FG_COLOR_MAGENTA "%d\n" ANSI_FG_COLOR_BLUE "Total time taken for transmission is " ANSI_FG_COLOR_MAGENTA "%ds\n" ANSI_COLOR_RESET,
           str_len, end - srt);
    for (int i = 0; i < seqmax; i++)
        printf("Sent pkt%d in %ds\n", i, data[i].ack_recv_time - data[i].sendtime);

    close(sockfd);

    // server_addr.sin_family = AF_INET;
    // server_addr.sin_port = htons(0);
    // server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // memset(&server_addr, 0, sizeof(server_addr));
    // printerror(bind(sockfd, (struct sockaddr *)&server_addr, server_addr_size), "Unbind error");
    // printf("[+]Unbound from port number: %d\n", port);
}

void client_code()
{
    server_addr_size = sizeof(server_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    printerror(sockfd, "Socket error");
    printf("[+]UDP server socket created.\n");
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    char buffer[PACKETSIZE] = {0};
    printerror(sendto(sockfd, buffer, PACKETSIZE, 0, (struct sockaddr *)&server_addr, server_addr_size), "Connection request");
    printf(ANSI_FG_COLOR_GREEN "Connection established\n" ANSI_COLOR_RESET);

    initialise_data();

    // obtain seqmax
    client_receive_custom(sockfd, buffer, &server_addr, &server_addr_size);
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
        if (client_is_all_sent())
            break;

        bzero(buffer, PACKETSIZE);
        if (client_receive_custom(sockfd, buffer, &server_addr, &server_addr_size) <= 0)
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
        client_print_pkt_recd();

        buffer[120] = 0;
        strcpy(data[seqnum].packet, buffer);
        sleep(TIME_BETWEEN_PACKETS);
        client_send_ack(sockfd, seqnum, &server_addr, server_addr_size);
        data[seqnum].ack_sent = 1;
        printf("ack%02d ", seqnum);
        client_print_ack_sent();

        sleep(TIME_BETWEEN_PACKETS);
    }

    end = time(NULL);

    // stitch all packets together
    char str[seqmax * DATASIZE];
    for (int i = 0; i < seqmax; i++)
        strncpy((str + i * DATASIZE), data[i].packet, DATASIZE);

    printf("\n"
           "Data received is\n"
           "**** ******** **\n");
    printf("%s\n\n", str);
    printf(ANSI_FG_COLOR_BLUE "Total bytes received is " ANSI_FG_COLOR_MAGENTA "%d\n" ANSI_FG_COLOR_BLUE "Total time taken for transmission is " ANSI_FG_COLOR_MAGENTA "%ds\n" ANSI_COLOR_RESET,
           strlen(str), end - srt);

    close(sockfd);
}

int main()
{
    char buffer[PACKETSIZE];

    while (1)
    {
        do
        {
            printf("Do you want to act as a server or a client? [s|c]\n");
            bzero(buffer, PACKETSIZE);
            read(0, buffer, 10);
        } while (buffer[0] != 'c' && buffer[0] != 's');

        if (buffer[0] == 'c')
            client_code();
        else
            server_code();

        do
        {
            printf("Do you want to continue? [y|n]\n");
            read(0, buffer, 5);
        } while (buffer[0] != 'y' && buffer[0] != 'n');
        if (buffer[0] == 'n')
            break;
        // memset(&server_addr, 0, sizeof(server_addr));
        // server_addr_size = 0;
    }
}