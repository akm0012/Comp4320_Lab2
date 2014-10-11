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

// Struct that will be received from the client
struct received_packet
{
	unsigned short TML;	// Total Message Length (2 bytes)
	unsigned short RID;	// Request ID (2 bytes)
	unsigned char operation;	//operation (1 byte)
	char message[MAX_MESSAGE_LEN];	// Message (Limited to 1 Kb)
} __attribute__((__packed__));

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
	
		printf("Packet Received!\n");

		// Add the null to terminate the string
		packet_in.message[numbytes - 5] = '\0';	// numbytes - 5: To compensate for header
		
		if (DEBUG) {
			printf("struct.tml = %d\n", ntohs(packet_in.TML));
			printf("struct.rid = %d\n", ntohs(packet_in.RID));
			printf("struct.op = %d\n", packet_in.operation);
			printf("struct.message = %s\n", packet_in.message);
		}

		// Here we check what operation the client wanted. 
		
		if (packet_in.operation == V_LENGTH)
		{				
			if (DEBUG) {
				printf("Operation: vLength requested.\n");
				printf("String to process: %s\n", packet_in.message);
				printf("The number of vowels in string '%s' is: %d\n", 
					packet_in.message, count_vowels(packet_in.message));
			}

			// Create packet that will be returned to the client
			tx_vLength packet_out_vLength;
			
			// Fill in the struct with TML, RID and vLength
			packet_out_vLength.TML = htons(((sizeof packet_out_vLength.TML) 
				+ (sizeof packet_out_vLength.RID) 
				+ (sizeof packet_out_vLength.vLength)));
			packet_out_vLength.RID = htons(ntohs(packet_in.RID));
			packet_out_vLength.vLength = htons(count_vowels(packet_in.message)); 		
	
			if (DEBUG) {
				printf("packet_out_vLength.TML: %d\n", ntohs(packet_out_vLength.TML));
				printf("packet_out_vLength.RID: %d\n", ntohs(packet_out_vLength.RID));
				printf("packet_out_vLength.VLength: %d\n", ntohs(packet_out_vLength.vLength));
			}

			if (DEBUG) {
				printf("Sending vLength packet to Client.\n"); 			
			}

			// 5. sendto
			if (sendto(sockfd, (char *)&packet_out_vLength, ntohs(packet_out_vLength.TML), 
				0, (struct sockaddr *)&their_addr, addr_len) == -1)
			{
				perror("sendto error");
				exit(1);
			}		
		}

		else if (packet_in.operation == DISEMVOWEL)
		{
			if (DEBUG) {
				printf("Operation: Disemvowelment requested.\n");
				printf("String to process: %s\n", packet_in.message);
				printf("The string '%s' disemvowled is: %s\n", 
					packet_in.message, disemvowel(packet_in.message));
			}

			// Create packet that will be returned to the client
			tx_disVowel packet_out_disVowel;
			
			// Fill in the struct with TML, RID and disVowel string
			strcpy(packet_out_disVowel.message, disemvowel(packet_in.message));

			// Need to do the strcpy first or the message size won't be correct.
			packet_out_disVowel.TML = htons((sizeof packet_out_disVowel.TML) 
				+ (sizeof packet_out_disVowel.RID) 
				+ (strlen(packet_out_disVowel.message)));

			packet_out_disVowel.RID = htons(ntohs(packet_in.RID));
			
			if (DEBUG) {
				printf("packet_out_disVowel.TML: %d\n", ntohs(packet_out_disVowel.TML));
				printf("packet_out_disVowel.RID: %d\n", ntohs(packet_out_disVowel.RID));
				printf("packet_out_disVowel.message: %s\n", packet_out_disVowel.message);
				printf("strlen(packet_out_disVowel.message): %d\n", (int)strlen(packet_out_disVowel.message));
			}
			
			if (DEBUG) {
				printf("Sending disVowel packet to Client.\n"); 			
			}

			// 5. sendto
			if (sendto(sockfd, (char *)&packet_out_disVowel, ntohs(packet_out_disVowel.TML), 
				0, (struct sockaddr *)&their_addr, addr_len) == -1)
			{
				perror("sendto error");
				exit(1);
			}		
		}

		// Error!
		else
		{
			fprintf(stderr, "Operation not reconized!.\n");
			break; 
		}
	
		printf("Responce Sent!\n");
		
		if (DEBUG) {
			printf("Sending Echo...\n");
		}
		
		close(sockfd);
	}
	return 0;
} 


// Support Functions

/*
* This function counts the number of vowels in a string.
* Assumes 'Y' is NOT a vowel.
*
* @param:	string_in: The string you want the number of vowels in.
* @return:	int: The number of voewls in the string.
*/  
int count_vowels(char* string_in)
{
	int vowel_count = 0;

	int i;
	for (i = 0; i < strlen(string_in); i++)
	{
		if (string_in[i] == 'a' || string_in[i] == 'A' ||
			string_in[i] == 'e' || string_in[i] == 'E' ||
			string_in[i] == 'i' || string_in[i] == 'I' ||
			string_in[i] == 'o' || string_in[i] == 'O' ||
			string_in[i] == 'u' || string_in[i] == 'U') 
		{
			vowel_count++;
		}
	}

	return vowel_count;
}

/*
* This function takes all the voewls out of a string.
* Assumes 'Y' is NOT a vowel.
*
* @param: string_in: The string you want to remove the vowels from.
* @return: char*: The string without vowels in it. 
*/
char* disemvowel(char* string_in) 
{
	if (DEBUG) {
		printf("Starting Disemvowelment\n");
	}
	
	int str_len = strlen(string_in);
	static char new_string[MAX_MESSAGE_LEN];	// Make static so interrupts won't clear the stack

	int i = 0;
	int j = 0;

	for (i = 0; i < strlen(string_in); i++)
	{

		if(0) {
			printf("Starting loop %d\n", i);
		}
	
		if (string_in[i] != 'a' && string_in[i] != 'A' &&
			string_in[i] != 'e' && string_in[i] != 'E' &&
			string_in[i] != 'i' && string_in[i] != 'I' &&
			string_in[i] != 'o' && string_in[i] != 'O' &&
			string_in[i] != 'u' && string_in[i] != 'U') 
			{
				new_string[j] = string_in[i];
				j++;
			} 
	}

	// End the string with '\0'
	new_string[j] = '\0';

	int new_len = strlen(new_string);
	
	if (DEBUG) {
		printf("disemvowel: size of old string (%s): %d\n", string_in, str_len);
		printf("disemvowel: size of new string (%s): %d\n", new_string, new_len);
	}

	return &new_string[0];
}

