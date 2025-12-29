/*
** Multicast sender
**
** Author: Edson Pereira, PY2SDR
**
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <stdint.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    char *multicastGroup;
    uint16_t multicastPort;
    struct sockaddr_in addr;
    socklen_t addrlen;
    int sock, multicastTTL, cnt;
    size_t n;
    char buffer[2048];

    if (argc < 3 || argc > 4)
    {
        fprintf(stderr, "Usage: %s <multicast_group> <port> [ttl]\n", argv[0]);
        fprintf(stderr, "Example: %s 239.0.0.11 15004 1\n", argv[0]);
        fprintf(stderr, "  ttl: Time-to-live (default: 1, range: 1-255)\n");
        exit(1);
    }

    multicastGroup = argv[1];
    multicastPort = atoi(argv[2]);

    if (multicastPort == 0)
    {
        fprintf(stderr, "Error: Invalid port number\n");
        exit(1);
    }

    if (argc > 3)
    {
        multicastTTL = atoi(argv[3]);
        if (multicastTTL < 1 || multicastTTL > 255)
        {
            fprintf(stderr, "Error: TTL must be between 1 and 255\n");
            exit(1);
        }
    }
    else
        multicastTTL = 1;

    /* set up socket */
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        exit(1);
    }

    bzero((char *)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(multicastGroup);
    addr.sin_port = htons(multicastPort);
    addrlen = sizeof(addr);

    /* Set TTL of multicast packet */
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL,
                   (void *) &multicastTTL, sizeof(multicastTTL)) < 0)
    {
        perror("setsockopt");
        exit(1);
    }

    while ((n = fread(buffer, sizeof(int16_t), 512, stdin)) > 0)
    {
        cnt = sendto(sock, buffer, n * sizeof(int16_t), 0,
                     (struct sockaddr *) &addr, addrlen);
        if (cnt < 0)
        {
            perror("sendto");
            break;
        }
    }

    close(sock);
    return 0;
}
