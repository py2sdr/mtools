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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdint.h>

int main(int argc, char *argv[])
{
   char *multicastGroup;
   uint16_t multicastPort;
   struct sockaddr_in addr;
   socklen_t addrlen;
   int sock, cnt, optval;
   struct ip_mreq mreq;
   char message[2048];

   if (argc != 3) {
       fprintf(stderr, "Usage: %s <multicast_group> <port>\n", argv[0]);
       fprintf(stderr, "Example: %s 239.0.0.11 15004\n", argv[0]);
       exit(1);
   }

   multicastGroup = argv[1];
   multicastPort = atoi(argv[2]);

   if (multicastPort == 0) {
       fprintf(stderr, "Error: Invalid port number\n");
       exit(1);
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

   mreq.imr_multiaddr.s_addr = inet_addr(multicastGroup);
   mreq.imr_interface.s_addr = htonl(INADDR_ANY);

   if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                  &mreq, sizeof(mreq)) < 0) {
       perror("setsockopt mreq");
       exit(1);
   }

   while (1) {
       addrlen = sizeof(addr);
       cnt = recvfrom(sock, message, sizeof(message), 0,
                      (struct sockaddr *) &addr, &addrlen);
       if (cnt < 0) {
           perror("recvfrom");
           exit(1);
       }
       write(1, message, cnt);
   }
}
