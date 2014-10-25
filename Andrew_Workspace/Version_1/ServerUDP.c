/*	
*	Comp4320: Lab 1
*
*	File: ServerUDP.c	
*	Author: Andrew K. Marshall (akm0012)
*	Group ID: 15
*	Date: 9/24/14
*	Version: 1.0
*	Version Notes: This version is able to read from the client, and then return a 
*	               packet with the correct response. 
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

#define GROUP_PORT "10025"	// Port should be 10010 + Group ID

#define MAX_MESSAGE_LEN 1024
#define MAX_PACKET_LEN 1029	// 1Kb for message, and 5 bytes for header
#define V_LENGTH 85	// Operation: Count vowels
#define DISEMVOWEL 170	// Operation: Remove vowels

#define DEBUG 0	// Used for debugging: 1 = ON; 0 = OFF

// Prototypes
int count_vowels(char*);
char* disemvowel(char*); 
unsigned char add_bits(unsigned char *, int); 

unsigned char calculate_checksum(unsigned char *data_in, int data_in_length); 
// Struct that will be received from the client
struct received_packet
{
	unsigned short TML;	// Total Message Length (2 bytes)
	unsigned char checksum;
	unsigned char GID;	//Group ID (1 byte)
	unsigned char RID;	// Request ID (1 byte)
	char message[MAX_MESSAGE_LEN];	// Message (Limited to 1 Kb)
} __attribute__((__packed__));

typedef struct received_packet rx_packet;

// Struct that will be used to recieve unverified incoming packets.
struct incoming_unverified_packet
{
	unsigned char b1;
	unsigned char b2;
	unsigned char b3;
	unsigned char b4;
	unsigned char b5;
	unsigned int extra[MAX_MESSAGE_LEN];
} __attribute__((__packed__));

typedef struct incoming_unverified_packet rx_check;
	
	

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

// Used to determine if we are using IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa) 
{
	if (sa->sa_family == AF_INET) 
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	//else
	return &(((struct sockaddr_in6*)sa)->sin6_addr);

}


int main(int argc, char *argv[])
{
	// Variables 
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int status;
	int numbytes;
	char *my_port;
	
	struct sockaddr_storage their_addr;
	char buf[MAX_PACKET_LEN];	// Make the buffer big enough for a full 1Kb message + 5 bytes for TML, etc.
	socklen_t addr_len;		

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;	// set to AF_INIT to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;	// use my IP

	// Check to make sure the command line arguments are valid
	if (argc != 2) 
	{
		fprintf(stderr, "Usage Error: Should be 1 argument: the port number.\n");
		exit(1);
	}

	// Get the port number from the command line
	my_port = argv[1];
	
	if (DEBUG) {
		printf("DEBUG: Port number: %s\n", my_port);
	}
	
	printf("Starting Server... to stop, press 'Ctrl + c'\n");
	
	while(1)
	{		
		// 1. getaddrinfo
		if ((status = getaddrinfo(NULL, my_port, 
			&hints, 	// points to a struct with info we have already filled in
			&servinfo)) != 0)	// servinfo: A linked list of results
		{
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
			return 1;
		}

		// Loop through all the results and bind to the first we can
		for (p = servinfo; p != NULL; p = p->ai_next)
		{
			// 2. socket
			if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
			{	
				perror("Socket Error!");
				continue;
			}
		
			// 3. bind
			if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
			{
				close(sockfd);
				perror("Bind Error!");
				continue;
			}

			break;
		}
		
		if (p == NULL)
		{
			fprintf(stderr, "Failed to bind socket\n");
			return 2;
		}	
		

		freeaddrinfo(servinfo); 	// Call this when we are done with the struct "servinfo"

		if (DEBUG) {
			printf("Binding complete, waiting to recvfrom...\n");
		}

		addr_len = sizeof their_addr;

		rx_packet packet_in;

		// 4. recvfrom
		// MAX_PACKET_LEN -1: To make room for '\0'
		if ((numbytes = recvfrom(sockfd, (char *) &packet_in, MAX_PACKET_LEN - 1, 0,
			(struct sockaddr *)&their_addr, &addr_len)) == -1)
		{
			perror("recvfrom");
			exit(1);
		}
	
		printf("Packet Received! It contained: %d bytes.\n", numbytes);

		// Add the null to terminate the string
		packet_in.message[numbytes - 5] = '\0';	// numbytes - 5: To compensate for header
		
//		if (DEBUG) {
			printf("struct.tml = %d\n", ntohs(packet_in.TML));
			printf("struct.checksum = %X\n", packet_in.checksum);
			printf("struct.gid = %d\n", packet_in.GID);
			printf("struct.rid = %d\n", packet_in.RID);
			printf("struct.message = %s\n", packet_in.message);
			printf("Checksum Check:  %X\n", calculate_checksum((unsigned char *)&packet_in, ntohs(packet_in.TML)));
//		}

			rx_check rx_verify;

			rx_verify.b1 = 0;

			rx_verify.b2 = 13;
			rx_verify.b3 = 0;
			rx_verify.b4 = 15;
			rx_verify.b5 = 99;
			rx_verify.extra[0] = 0xffffffff;
			rx_verify.extra[1] = 0xff1100aa;
			rx_verify.b3 = calculate_checksum((unsigned char *)&rx_verify, 5 + 8);

			printf("----- Sent Packet -----\n");
			printf("rx_verify.b1: \t%d \t(%X)\n", rx_verify.b1, rx_verify.b1);
			printf("rx_verify.b2: \t%d \t(%X)\n", rx_verify.b2, rx_verify.b2);
			printf("rx_verify.b3: \t%d \t(%X)\n", rx_verify.b3, rx_verify.b3);
			printf("rx_verify.b4: \t%d \t(%X)\n", rx_verify.b4, rx_verify.b4);
			printf("rx_verify.b5: \t%d \t(%X)\n", rx_verify.b5, rx_verify.b5);
			printf("rx_verify.extra: \t%o\n\n", rx_verify.extra[0]);
			printf("rx_verify.extra: \t%o\n\n", rx_verify.extra[1]);


			// 5. sendto
			if (sendto(sockfd, (char *)&rx_verify, 5 + 8, 
				0, (struct sockaddr *)&their_addr, addr_len) == -1)
			{
				perror("sendto error");
				exit(1);
			}		
		close(sockfd);
	}
	return 0;
} 


// Support Functions

/*
* This function calculates the 1-complement checksum
*
*/
unsigned char calculate_checksum(unsigned char *data_in, int data_in_length) 
{

	int checksum = 0;
	int carry = 0;
	int i;

	for (i = 0; i < data_in_length; i++) 
	{
//		printf("Data_in[%i]: \t%X\n", i, (int)data_in[i]);
		checksum += (int) data_in[i];
		carry = checksum >> 8;
		checksum = checksum & 0xFF;
//		printf("Before - i:%i \tcarry: %X\t checksum: %X\n", i, carry, checksum);
		checksum = checksum + carry;
//		printf("After - i:%i \tcarry: %X\t checksum: %X\n", i, carry, checksum);
	}

//	if (DEBUG) {
//		printf("Real Sum: %X\n", checksum);
//	}

	checksum = ~checksum;

//	if (DEBUG) {
//		printf("Checksum: %X\n", checksum);
//	}

	return (unsigned char) checksum;
}
