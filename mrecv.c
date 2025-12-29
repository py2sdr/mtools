/*
** Multicast receiver
**
** Author: Edson Pereira, PY2SDR
**
*/
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

void usage(char *progname)
{
    fprintf(stderr, "Usage: %s -a <multicast_group> -p <port> -i <interface>\n", progname);
    fprintf(stderr, "Example: %s -a 239.0.0.11 -p 15004 -i dummy0\n", progname);
    fprintf(stderr, "\nRequired arguments:\n");
    fprintf(stderr, "  -a <multicast_group>  Multicast IP address\n");
    fprintf(stderr, "  -p <port>             UDP port number\n");
    fprintf(stderr, "  -i <interface>        Network interface name (e.g., dummy0, eth0)\n");
    exit(1);
}

int main(int argc, char *argv[])
{
   char *multicastGroup = NULL;
   char *interface = NULL;
   uint16_t multicastPort = 0;
   struct sockaddr_in addr;
   socklen_t addrlen;
   int sock, cnt, optval, opt;
   struct ip_mreq mreq;
   char message[2048];

   /* Parse command line arguments */
   while ((opt = getopt(argc, argv, "a:p:i:h")) != -1)
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
   if (sock < 0) {
     perror("socket");
     exit(1);
   }

   optval = 1;
   if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
       perror("setsockopt SO_REUSEADDR");
   }
#ifdef SO_REUSEPORT
   if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
       perror("setsockopt SO_REUSEPORT");
   }
#endif

   bzero((char *)&addr, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(multicastPort);

   /* receive */
   if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
       perror("bind");
       exit(1);
   }

   /* Get interface address */
   struct ifreq ifr;
   strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
   ifr.ifr_name[IFNAMSIZ - 1] = '\0';

   if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
   {
       perror("ioctl SIOCGIFADDR");
       fprintf(stderr, "Error: Cannot get address for interface %s\n", interface);
       exit(1);
   }

   mreq.imr_multiaddr.s_addr = inet_addr(multicastGroup);
   mreq.imr_interface.s_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

   if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                  &mreq, sizeof(mreq)) < 0) {
       perror("setsockopt mreq");
       exit(1);
   }

   while (1)
   {
       addrlen = sizeof(addr);
       cnt = recvfrom(sock, message, sizeof(message), 0,
                      (struct sockaddr *) &addr, &addrlen);
       if (cnt < 0) {
           perror("recvfrom");
           break;
       }
       write(1, message, cnt);
   }

   close(sock);
   return 0;
}
