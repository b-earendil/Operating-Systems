///////////////////////////////////////////////////////////////////////////////////////////////
//
// INCLUDES
//
///////////////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/errno.h>
#include <stddef.h>

#include "a5-header.h"

///////////////////////////////////////////////////////////////////////////////////////////////
//
// PROTOTYPES
//
///////////////////////////////////////////////////////////////////////////////////////////////
struct cmdline parse(int argc, char *argv[]);
void usage();
void initHints(struct addrinfo* hints, char* socketType);
void* getSocketIPAddress(struct sockaddr* socket_address_info);
int sendall(int s, char* buf,int * len);

///////////////////////////////////////////////////////////////////////////////////////////////
//
// DEFINES
//
///////////////////////////////////////////////////////////////////////////////////////////////
#define MAX 10

///////////////////////////////////////////////////////////////////////////////////////////////
//
// MAIN
//
///////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    struct cmdline clientCmdOptions;
    struct addrinfo hints, *clientInfo, *curr;
    struct timeval tv;
    char addrString[INET6_ADDRSTRLEN], buf[MAX];
    int status, clientSocket = 0, numberBytes = sizeof(struct packet);
    socklen_t addrLength = sizeof(struct sockaddr_in);

    // Parse command line, initialize hints, set timeout value
    clientCmdOptions = parse(argc, argv);
    initHints(&hints, clientCmdOptions.socketType);
    tv.tv_sec = 3; // 3 second timeout

    // Pack the message
    struct packet dp;
    printf("clientCmdOptions.data %d\n", clientCmdOptions.data);
    dp.number = htonl(clientCmdOptions.data);
    dp.version = 0x1;
    memcpy(&buf, &dp, 5);

    // Load address structs with getaddrinfo()
    if((status = getaddrinfo(inet_ntoa(clientCmdOptions.ipv4), clientCmdOptions.port, &hints, &clientInfo)) != 0) {
        fprintf(stderr, "Client - Failure with getaddrinfo()");
        fprintf(stderr, "Client - %s\n", gai_strerror(status));
        return 1;
    }

    // Create socket
    for(curr = clientInfo; curr != 0; curr = curr->ai_next) {
        if((clientSocket = socket(curr-> ai_family, curr->ai_socktype, curr->ai_protocol)) == -1) {
            perror("Client - Failure with socket()");
            continue; // Try again
        }

        // If TCP selected, connect it to socket as well
        if(hints.ai_socktype == SOCK_STREAM) {
            printf("tcp so connect\n");
            if((connect(clientSocket, curr->ai_addr, curr->ai_addrlen) == -1)) {
                close(clientSocket);
                perror("Client - Failure with connect() to TCP");
                continue; // Try again
            }
            printf("socket created\n");
        }
        break;
    }
    
    // Terminate if socket failed
    if(curr == 0 && hints.ai_socktype == SOCK_STREAM) {
        fprintf(stderr, "Client - Failure to connect via TCP\n");
        freeaddrinfo(clientInfo);
        exit(1);
    }
    else if(curr == 0 && hints.ai_socktype == SOCK_DGRAM) {
        fprintf(stderr, "Client - Failure to connect via UDP\n");
        freeaddrinfo(clientInfo);
        exit(1);
    }

    //
    // TCP
    //
    if(hints.ai_socktype == SOCK_STREAM) {
        // Use setsockopt to set timeout value of 3 seconds
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));

        // Convert from network byte order
        inet_ntop(clientInfo->ai_family, getSocketIPAddress((struct sockaddr*) clientInfo->ai_addr), addrString, sizeof(addrString));
        printf("Client - Connected with %s:%s!", addrString, clientCmdOptions.port);

        //
        // Communicate with the server
        //

        // Send
        if((sendall(clientSocket, buf, &numberBytes)) == -1) {
            perror("Client - Failure with sendall()");
            freeaddrinfo(clientInfo);
            close(clientSocket);
            exit(1);
        }
        printf("\nClient - %d bytes were sent to the server at %s:%s via the TCP protocol\n", numberBytes, inet_ntoa(clientCmdOptions.ipv4), clientCmdOptions.port);
        printf("Client - sent %d to the server %s:%s\n", clientCmdOptions.data, inet_ntoa(clientCmdOptions.ipv4), clientCmdOptions.port);
        // Receive
        printf("Client - Eagerly awaiting a reply from the server...\n");
        if((numberBytes = recv(clientSocket, buf, sizeof(u_int8_t), 0)) == -1) {
            printf("Client - Failure with recv()\n");
            fprintf(stderr, "Client - Failure with recv()\n");
            freeaddrinfo(clientInfo);
            close(clientSocket);
            exit(1);
        }

        // Display server's response 
        printf("Client - %d bytes have been successfully received from the server\n", numberBytes);
        printf("Client - Reply: %d Success!\n\n", buf[0]);
    }

    //
    // UDP Protocol
    //
    if(hints.ai_socktype == SOCK_DGRAM) {
        // Set timeout
        setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (struct timeval*)&tv, sizeof(struct timeval));

        // Send
        if((numberBytes = sendto(clientSocket, buf, sizeof(struct packet), 0, clientInfo->ai_addr, clientInfo->ai_addrlen)) == -1) {
            perror("Client - Failure with sendto()");
            printf("Client - Failure with sendto()\n");
            freeaddrinfo(clientInfo);
            close(clientSocket);
            exit(1);
        }
        printf("Client - %d bytes sent to server %s:%s using UDP\n", numberBytes, inet_ntoa(clientCmdOptions.ipv4), clientCmdOptions.port);
        // Receive
        printf("Client - Eagerly awaiting the server's response\n");
        if((numberBytes = recvfrom(clientSocket, buf, sizeof(struct packet), 0, clientInfo->ai_addr, &addrLength)) == -1) {
            printf("Client - Failure with recvfrom()\n");
            perror("Client - Failure with recvfrom");
            freeaddrinfo(clientInfo);
            close(clientSocket);
            exit(1);
        }
        printf("Client - %d bytes received\n", numberBytes);
        printf("Reply from server %d\n", buf[0]);
        close(clientSocket);
    }

    return 0;
}


///////////////////////////////////////////////////////////////////////////////////////////////
//
// HELPERS
//
///////////////////////////////////////////////////////////////////////////////////////////////
struct cmdline parse(int argc, char *argv[]) {
    struct cmdline clientCmdOptions;
    int option = 0;
    int countOfOptions = 0;

    // Populate clientCmdOptions 
    while((option = getopt(argc, argv, "x: t: s: p:")) != -1) {
        switch(option) {
            case 'x':
                clientCmdOptions.data = atoi(optarg);
                countOfOptions++;
                break;
            case 't':
                strncpy(clientCmdOptions.socketType, optarg, 3);
                countOfOptions++;
                break;
            case 's':
                // Verify IPv4 address
                if (inet_pton(AF_INET, optarg, &clientCmdOptions.ipv4)) {
                    clientCmdOptions.hostName = 0; // set to NULL
                    countOfOptions++;
                }
                else { // valid hostname
                    clientCmdOptions.ipv4.s_addr = 0;
                    clientCmdOptions.hostName = optarg;
                    countOfOptions++;
                }
                break;
            case 'p':
                clientCmdOptions.port = optarg;
                countOfOptions++;
                break;
            default:
                usage();
        }
    }
    if(countOfOptions !=4) {
        printf("Please enter the correct number of parameters\n");
        usage();
    }
    return clientCmdOptions;
}


void usage() {
    printf("\nUsage - <-Option> <Value>\n\n");
    printf("\t-x <data>\n");
    printf("\t-t <tcp> or <udp> to specify protocol\n");
    printf("\t-s <ip>\n");
    printf("\t-p <port> number\n");
    exit(0);
}


void initHints(struct addrinfo* hints, char* socketType) {
    memset(hints, 0, sizeof(*hints));
    hints->ai_family = AF_INET; // IPv4
    if(strncmp(socketType, "tcp", 3)==0) {
        hints->ai_socktype = SOCK_STREAM; // TCP
    } 
    else if(strncmp(socketType, "udp", 3)==0) {
        hints->ai_socktype = SOCK_DGRAM; // UDP
    }
}

void* getSocketIPAddress(struct sockaddr* socket_address_info) {
    return &(((struct sockaddr_in*)socket_address_info)->sin_addr);
}

int sendall(int s, char* buf, int * len) {
    int total = 0;
    int bytesLeft = *len;
    int n = 0;

    while(total < *len) {
        n = send(s, buf+total, bytesLeft, 0);
        if(n == -1) { break; }
        total += n;
        bytesLeft -= n;
    }

    *len = total; // return the number actually sent here
    printf("length sent %d\n", *len);
    return n==-1?-1:0; // return -1 on failure, 0 on success
}
