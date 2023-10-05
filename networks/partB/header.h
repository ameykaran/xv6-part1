#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <time.h>
#include <fcntl.h>

#define PACKETSIZE 128
#define DATASIZE 120
#define SEQSIZE 120
#define MAXPACKETS 256
#define TIME_BETWEEN_PACKETS 1
#define TIME_WAIT 3
#define TIME_CLOSE 5

#define min(a, b) a < b ? a : b

#define ANSI_FG_COLOR_GREEN "\x1b[32m"
#define ANSI_FG_COLOR_YELLOW "\x1b[33m"
#define ANSI_FG_COLOR_BLUE "\x1b[34m"
#define ANSI_FG_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_RESET "\x1b[0m"

/*
struct packet
{
    char data[DATASIZE];
    int seqNum; // 256 packets can be sent
}
*/

#define printerror(func, msg) \
    if (func < 0)             \
    {                         \
        perror("[-]" msg);    \
        exit(1);              \
    }
