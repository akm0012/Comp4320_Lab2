/*
** talker.c -- a datagram "client" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAXBUFLEN 100
#define SERVERPORT "10025"    // the port users will be connecting to
#define MAX_MESSAGE_LEN 1024

// Struct that will be used to send data to the Server (Alpha version)
struct packet_to_send
{
	unsigned short TML;
	unsigned short RID;
	unsigned char operation;
	char message[MAX_MESSAGE_LEN];
} __attribute__((__packed__));

typedef struct packet_to_send tx_packet;

typedef struct received_packet rx_packet;	
	

// Struct that will be sent to the client, if client requested vLength
struct transmitted_packet_vLength
{
	unsigned short TML;	// Total Message Length (2 bytes)
	unsigned short RID;	// Request ID (2 bytes)
	unsigned short vLength;	// The number of vowels 
}__attribute__((__packed__));	

typedef struct transmitted_packet_vLength tx_vLength;

// Struct that will be sent to the client, if client requested diemvowelment
struct transmitted_packet_disemvowel
{
	unsigned short TML;	// Total Message Length (2 bytes)
	unsigned short RID;	// Request ID (2 bytes)
	char message[MAX_MESSAGE_LEN];	// The number of vowels 
}__attribute__((__packed__));	

typedef struct transmitted_packet_disemvowel tx_disVowel;

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes_tx;
	int numbytes_rx;
	char buf[MAXBUFLEN];
	struct sockaddr_storage their_addr;
	socklen_t addr_len;
	

    if (argc != 3) {
        fprintf(stderr,"usage: talker hostname message\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);	// put 0's in all the mem space for hints (clearing hints)
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) 
		{
            perror("talker: socket");
            continue;
		}
/*
		//Add code to bind as well
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
		{
			close(sockfd);
			perror("talker: bind");
			continue;
		}        

*/
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to bind socket\n");
        return 2;
    }

	// Creating the struct to send!!
	int tml = 26; // Include NULL '\0'
	int rid = 1;
	char op = 170; // 85 (0x55)= vLength (need to make this a constant), 170 (0xAA) = disemvowelment
	char message_in[] = "AHelloMyNameIsAndrewA";
	//char message_in[] = "g";

	tx_packet test_packet;
	test_packet.TML = htons(tml);
	test_packet.RID = htons(rid);
	test_packet.operation = op; // DO NOT need to use htons if it is only 1 byte
	strcpy(test_packet.message, message_in);	// must use strcpy


   // if ((numbytes_tx = sendto(sockfd, argv[2], strlen(argv[2]), 0,
    //         p->ai_addr, p->ai_addrlen)) == -1) {
	if ((numbytes_tx = sendto(sockfd, (char *)&test_packet, ntohs(test_packet.TML), 0, 
		p->ai_addr, p->ai_addrlen)) == -1) 
	{    
		perror("talker: sendto");
        exit(1);
    }

//    freeaddrinfo(servinfo);

    printf("talker: sent %d bytes to %s\n", numbytes_tx, argv[1]);
    
	//Adding code to try and recieve an echo

	printf("talker: waiting for conformation echo...\n");

	addr_len = sizeof their_addr;
if(0) {
	tx_vLength tx_v_pac;

	if ((numbytes_rx = recvfrom(sockfd,(char *) &tx_v_pac, 1029, 0, 
		(struct sockaddr *)&their_addr, &addr_len)) == -1)
	//if ((numbytes_rx = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, 
	//	p->ai_addr, p->ai_addrlen)) == -1)
	{
		perror("recvfrom");
		exit(1);
	}

	printf("talker: received echo, packet is %d bytes long.\n", numbytes_rx);

	printf("tx_v_pac.TML = %d\n", ntohs(tx_v_pac.TML));
	printf("tx_v_pac.RID = %d\n", ntohs(tx_v_pac.RID));
	printf("tx_v_pac.vLength = %d\n", ntohs(tx_v_pac.vLength));
}	
if(1) {
	tx_disVowel tx_dV_pac;

	if ((numbytes_rx = recvfrom(sockfd,(char *) &tx_dV_pac, 1029, 0, 
		(struct sockaddr *)&their_addr, &addr_len)) == -1)
	//if ((numbytes_rx = recvfrom(sockfd, buf, MAXBUFLEN-1, 0, 
	//	p->ai_addr, p->ai_addrlen)) == -1)
	{
		perror("recvfrom");
		exit(1);
	}

	printf("talker: received echo, packet is %d bytes long.\n", numbytes_rx);

	printf("tx_dV_pac.TML = %d\n", ntohs(tx_dV_pac.TML));
	printf("tx_dV_pac.RID = %d\n", ntohs(tx_dV_pac.RID));
	printf("tx_dV_pac.message = %s\n", tx_dV_pac.message);
}

    freeaddrinfo(servinfo);
	close(sockfd);

    return 0;
}


































