/*
 *  Client.h
 *  nettest
 *
 *  Created by Keith Driver on 10/03/2011.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#define MAX_LEN 100
#define MCPORT 7654
#define TCPPORT 14141

#define MESSAGE_SIZE 100
#define PACKET_SIZE  1024

#define PAYLOAD_5MB   (unsigned long)5000000

void BeAClient(void);