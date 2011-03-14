#include <stdio.h>
#include <stdlib.h>      /* for atoi() */
#include <string.h>      /* for strlen() */
#include <unistd.h>      /* for close() */
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>   /* for type definitions */
#include <sys/socket.h>  /* for socket API function calls */
#include <netinet/in.h>  /* for address structs */
#include <arpa/inet.h>   /* for sockaddr_in */

#include "Client.h"

#define MIN_PORT 1024    /* minimum port allowed */
#define MAX_PORT 65535   /* maximum port allowed */


enum mode_t {
	CLIENT,SERVER
} mode;

#define  CLIENT 1
#define  SERVER 2

//   local socket

unsigned char mc_ttl=1;     /* time to live (hop count) */
struct sockaddr_in mc_addr; /* socket address structure */
char send_str[MAX_LEN];     /* string to send */
unsigned int send_len;      /* length of string to send */
char* mc_addr_str = "225.0.0.1";          /* multicast IP address */
unsigned short mc_port = MCPORT;     /* multicast port */

void sendMCPacket(void)
{
	int sock;
	
	
	
	/* create a socket for sending to the multicast address */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("socket() failed");
		exit(1);
	}
	
	/* set the TTL (time to live/hop count) for the send */
	if ((setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, 
					(void*) &mc_ttl, sizeof(mc_ttl))) < 0) {
		perror("setsockopt() failed");
		exit(1);
	} 
	
	/* construct a multicast address structure */
	memset(&mc_addr, 0, sizeof(mc_addr));
	mc_addr.sin_family      = AF_INET;
	mc_addr.sin_addr.s_addr = inet_addr(mc_addr_str);
	mc_addr.sin_port        = htons(MCPORT);
	
	/* clear send buffer */
	memset(send_str, 0, sizeof(send_str));
	
	send_len = 10;
	
	/* send string to multicast address */
    if ((sendto(sock, send_str, send_len, 0, 
				(struct sockaddr *) &mc_addr, 
				sizeof(mc_addr))) != send_len) {
		perror("sendto() sent incorrect number of bytes");
		exit(1);
	}
		
	close(sock);
}

int bindlocal(void)
{

	int sockfd;
	int yes=1;
	struct sockaddr_in my_addr;
    int flags=0;
	
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	
	
	if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	} 
    
    
     if ( (flags=fcntl(sockfd,F_GETFL,0)) < 0 )
     {
         perror("flags error\n");
     }
    else
    {
        if ( fcntl(sockfd,F_SETFL,flags | O_NONBLOCK ) < 0 )
        {
            perror("error setting non blocking I/O\n");
        }
        else
            printf("non blocking i/o enabled\n");
    }
        
               
               
               
    
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(TCPPORT);     // short, network byte order
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);
	
	if ( bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) < 0 )
	{
		perror("bind() failed\n");
		exit(1);
	}
    else
        printf("waiting on port %d\n",TCPPORT);
	
	
	
	if ( listen(sockfd,10 ) < 0 )
	{
		perror("listen() error \n");
		exit(1);
	}
	
	return sockfd;
}

void  HandleClient(int fd)
{
	int desc;
	char command[MESSAGE_SIZE]; 
	char packet[PACKET_SIZE];
	int i;
    int s;
    unsigned long left_to_send=0;
	
	desc = fd;
	
	// OK, the client is going to make a request so lets get the request
	
	if ( recv(desc, command, sizeof(command), 0) != MESSAGE_SIZE )
	{
		printf("not enough bytes in command\n");
	}
	else {
		if ( strcmp("SIZE 50000",command) == 0 )
		{
			int total = 0;
			int attempts=0;
			i = 0;
			printf("conduct test , sending %d bytes\n",50000*sizeof(packet));
			
			while ( total < (50000*sizeof(packet)) )
			{
					
				attempts++;
				s=send(desc, packet, sizeof(packet), 0);
				if ( s < 0 )
				{
					if ( errno == EAGAIN )
					{
						usleep(10000);
						printf(".");
						continue;
					}
					else
					{
						perror("send\n");
					}
				}
				else
				{
					total = total + s;
					i++;
				}
						

				if ( (i%1000) == 0 )
					printf("sent %d packets\n",i);
			}
			printf("send %d bytes in %d packets in %d attempts\n",total,i,attempts);
			
			// we are going to send 50000 messages of data so the client can measure the speed
			
		}
	}
	printf("done - wait for next client \n");

	
	
	
}

int main (int argc, const char * argv[]) {
	
	int i=0;
	int sock;
	
	mode = SERVER;
	if ( argc > 1 )
	{
		for ( i = 1 ; i < argc ; i++ )
		{
			if ( !strcmp("-c",argv[i]) )
				{
					mode = CLIENT;
				}
			
		}
	}

	if ( mode == CLIENT )
	{
		printf("Client mode\n");
		BeAClient();
	}
	else {
		printf("Server mode\n");
	}


	sock = bindlocal();

	// Say we are here

	for ( i = 0 ; i < 5  ; i++ )
	{
		sendMCPacket();
		usleep(10000);
	}
	printf("%d Multicast packets sent \n",i);
	
	while ( 1 )
	{
		unsigned int sin_size;
		struct sockaddr_in their_addr;
		int new_fd=-1;


		printf("waiting for a client\n");
		sin_size = sizeof their_addr;
        
        while ( new_fd < 0 )
        {
            new_fd = accept(sock, (struct sockaddr *)&their_addr, &sin_size);
            if (new_fd == -1) {
                if ( errno == EAGAIN )
                {
		    printf(" No client, so send out another mc packet\n");
                    sendMCPacket();
                    usleep(1000000);
                }
                else
                {
                    perror("accept");
                    continue;
                }
            }
        }
		
        printf("server: got connection from %s\n", inet_ntoa(their_addr.sin_addr));
		
		// what to do this in a thread, but for now
		
		HandleClient(new_fd);
		
	}
	
	
	
    return 0;
}
