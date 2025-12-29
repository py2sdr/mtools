/*
** Multicast sender
**
** Author: Edson Pereira, PY2SDR
**
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void usage(char *progname)
{
    fprintf(stderr, "Usage: %s -a <multicast_group> -p <port> -i <interface> [-t <ttl>]\n", progname);
    fprintf(stderr, "Example: %s -a 239.0.0.11 -p 15004 -i dummy0\n", progname);
    fprintf(stderr, "Example: %s -a 239.0.0.11 -p 15004 -i dummy0 -t 2\n", progname);
    fprintf(stderr, "\nRequired arguments:\n");
    fprintf(stderr, "  -a <multicast_group>  Multicast IP address\n");
    fprintf(stderr, "  -p <port>             UDP port number\n");
    fprintf(stderr, "  -i <interface>        Network interface name (e.g., dummy0, eth0)\n");
    fprintf(stderr, "\nOptional arguments:\n");
    fprintf(stderr, "  -t <ttl>              Time-to-live (default: 1, range: 1-255)\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    char *multicastGroup = NULL;
    char *interface = NULL;
    uint16_t multicastPort = 0;
    struct sockaddr_in addr;
    struct in_addr localInterface;
    socklen_t addrlen;
    int sock, multicastTTL = 1, cnt, opt;
    size_t n;
    char buffer[2048];

    /* Parse command line arguments */
    while ((opt = getopt(argc, argv, "a:p:i:t:h")) != -1)
    {
        switch (opt)
        {
            case 'a':
                multicastGroup = optarg;
                break;
            case 'p':
                multicastPort = atoi(optarg);
                break;
            case 'i':
                interface = optarg;
                break;
            case 't':
                multicastTTL = atoi(optarg);
                if (multicastTTL < 1 || multicastTTL > 255)
                {
                    fprintf(stderr, "Error: TTL must be between 1 and 255\n");
                    exit(1);
                }
                break;
            case 'h':
            default:
                usage(argv[0]);
        }
    }

    /* Check required arguments */
    if (multicastGroup == NULL || multicastPort == 0 || interface == NULL)
    {
        fprintf(stderr, "Error: Missing required arguments\n\n");
        usage(argv[0]);
    }

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
        perror("setsockopt IP_MULTICAST_TTL");
        exit(1);
    }

    /* Set outgoing interface */
    struct ifreq ifr;

    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
    {
        perror("ioctl SIOCGIFADDR");
        fprintf(stderr, "Error: Cannot get address for interface %s\n", interface);
        exit(1);
    }

    localInterface.s_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
                   (void *)&localInterface, sizeof(localInterface)) < 0)
    {
        perror("setsockopt IP_MULTICAST_IF");
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
