/*
 *  Client.c
 *  nettest
 *
 *  Created by Keith Driver on 10/03/2011.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include <sys/types.h>  /* for type definitions */
#include <sys/socket.h> /* for socket API calls */
#include <netinet/in.h> /* for address structs */
#include <arpa/inet.h>  /* for sockaddr_in */
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() */
#include <string.h>     /* for strlen() */
#include <unistd.h>     /* for close() */
#include <sys/time.h>


#include "Client.h"

enum mc_t {
	FOUND,NOTFOUND
} mc_t;

struct sockaddr_in from_addr; /* packet source */


int GetServerFromMC(void)
{
	int sock;                     /* socket descriptor */
	int flag_on = 1;              /* socket option flag */
	struct sockaddr_in mc_addr;   /* socket address structure */
	char recv_str[MAX_LEN+1];     /* buffer to receive string */
	int recv_len;                 /* length of string received */
	struct ip_mreq mc_req;        /* multicast request structure */
	char* mc_addr_str="225.0.0.1";/* multicast IP address */
	unsigned int from_len;        /* source addr length */
	
	/* create socket to join multicast group on */
	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("socket() failed");
		exit(1);
	}
	
	/* set reuse port to on to allow multiple binds per host */
	if ((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag_on,
					sizeof(flag_on))) < 0) {
		perror("setsockopt() failed");
		exit(1);
	}
	 
	
	/* construct a multicast address structure */
	memset(&mc_addr, 0, sizeof(mc_addr));
	mc_addr.sin_family      = AF_INET;
	mc_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	mc_addr.sin_port        = htons(MCPORT);
	
	/* bind to multicast address to socket */
	if ((bind(sock, (struct sockaddr *) &mc_addr, 
			  sizeof(mc_addr))) < 0) {
		perror("bind() failed");
		exit(1);
	}
	
	/* construct an IGMP join request structure */
	mc_req.imr_multiaddr.s_addr = inet_addr(mc_addr_str);
	mc_req.imr_interface.s_addr = htonl(INADDR_ANY);
	
	/* send an ADD MEMBERSHIP message via setsockopt */
	if ((setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
					(void*) &mc_req, sizeof(mc_req))) < 0) {
		perror("setsockopt(add) failed");
		exit(1);
	}
	printf("looking for MC packet on %d\n",MCPORT);
	for (;;) {          /* loop forever */
		
		/* clear the receive buffers & structs */
		memset(recv_str, 0, sizeof(recv_str));
		from_len = sizeof(from_addr);
		memset(&from_addr, 0, from_len);
		
		/* block waiting to receive a packet */
		if ((recv_len = recvfrom(sock, recv_str, MAX_LEN, 0, 
								 (struct sockaddr*)&from_addr, &from_len)) < 0) {
			perror("recvfrom() failed");
			exit(1);
		}
		
		/* output received string */
		printf("Received %d bytes from %x: \n", recv_len, 
			   from_addr.sin_addr.s_addr);
		//printf("%s", recv_str);
		break;
	}
	
	/* send a DROP MEMBERSHIP message via setsockopt */
	if ((setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, 
					(void*) &mc_req, sizeof(mc_req))) < 0) {
		perror("setsockopt() failed");
	}
	
	close(sock);  

	
	return FOUND;
}

int
timeval_subtract(struct timeval *result,
				 struct timeval *x,
				 struct timeval *y)
{
	/* Perform the carry for the later subtraction by updating Y. */
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
	
	/* Compute the time remaining to wait.
     `tv_usec' is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;
	
	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}
void BeAClient()
{
	int sockfd;
	int ans;
	unsigned long total=0;
	int packets=0;
	struct timeval start_time,end_time,result;

	struct sockaddr_in other_addr;
	float tm;
	
	char msg[MESSAGE_SIZE];
	char packet[PACKET_SIZE];
	
	// IF we are lucky we will see a multicast packet
	
	if ( GetServerFromMC() != FOUND )
	{
		printf("need to get a realIP address\n");
		exit(0);
	}
	
	printf("so now connect to %s port %d\n",inet_ntoa(from_addr.sin_addr),TCPPORT);
    
	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	
	other_addr.sin_family = AF_INET;
	other_addr.sin_port = htons(TCPPORT);     // short, network byte order
	other_addr.sin_addr.s_addr = (from_addr.sin_addr.s_addr);
	memset(other_addr.sin_zero, '\0', sizeof other_addr.sin_zero);

	// connect!
	
	if ( connect(sockfd, (const struct sockaddr *)&other_addr , sizeof(other_addr)) < 0 )
	{
		perror("could not connect\n");
		exit(1);
	}
	else {
		printf("connected\n");
	}

	strcpy(msg,"SIZE 50000");
	
	if ( send(sockfd, msg, sizeof(msg), 0) != sizeof(msg)  )
	{
		printf("send error\n");
	}
	else {
		printf("sent client request\n");
	}
	// OK, now we expect that we will get 50000 packets of data of fixed size, so we need to keep reading unikt we have all the data
		
	gettimeofday(&start_time,NULL);
    
	unsigned long pc=0,last_pc=0,shortp=0,normalp=0;
    do {
		ans = recv(sockfd,packet,sizeof(packet),0);
		if ( ans < 0 )
		{
			perror("recv message failed\n");
			exit(1);
		}
		packets++;
		total = total + ans;
        if ( ans < sizeof(packet) )
        {
         //   printf("Small packet rxed %d, total %lu   :  \r ",ans,total);
            shortp++;
        }
        else
        {
            normalp++;
        }
        pc = (total*100)/(PAYLOAD_5MB);
        if ( pc != last_pc )
        {
            printf("  %lu short %lu normal  :  %lu  complete, total %lu\r",shortp,normalp,pc,total);
            fflush(stdout);
            last_pc = pc ;
        }
		
	}while (total < PAYLOAD_5MB) ;
	gettimeofday(&end_time,0);
	
	printf("\ngot all the data in %d read attempts ( %lu bytes )\n",packets,total);
	
	timeval_subtract(&result,&end_time,&start_time);
	tm=((float)result.tv_usec + (result.tv_sec * 1000000 ))/1000000;
	printf(" took   %f seconds\n",tm);
	printf(" which is %f bytes/sec\n",((float)total)/ tm);
	printf(" which is %f Mb/sec\n",((float)total)/1000000/ tm);

	sleep(5);
	exit(0);
	
}

