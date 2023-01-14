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
// DEFINES
//
///////////////////////////////////////////////////////////////////////////////////////////////
#define BACKLOG 10
#define MAX 100

///////////////////////////////////////////////////////////////////////////////////////////////
//
// PROTOTYPES
//
///////////////////////////////////////////////////////////////////////////////////////////////
struct cmdline parse(int argc, char* argv[]);
void usage();
void initHints(struct addrinfo* hints, char* socketType);
void initSigHandler(struct sigaction* sa);
void sigchld_handler(int s);
void* getSocketIPAddress(struct sockaddr* socket_address_info);
int sendall(int s, char* buf,int * len);

///////////////////////////////////////////////////////////////////////////////////////////////
//
// MAIN
//
///////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
    struct cmdline serverCmdOptions;
    struct addrinfo hints, *servInfo, *curr;
    struct sigaction sa;
    int status, serverSocket, yes = 1, conn, numberBytes = 0;
    char addrStr[INET_ADDRSTRLEN], buf[MAX];
    struct sockaddr_in addr;
    struct sockaddr_in addr2;
    socklen_t addrLen = sizeof(struct sockaddr_in);
    socklen_t addr2Len = sizeof(struct sockaddr_in);
    uint8_t serverReply = 1;
    
    // Parse cmdline, initialize hints, initialize sigchld_handler
    serverCmdOptions = parse(argc, argv);
    initHints(&hints, serverCmdOptions.socketType);
    initSigHandler(&sa);
    // printf("\n After Parse() : %s - protocol, %s - port\n\n", serverCmdOptions.socketType, serverCmdOptions.port);
    // printf("\n After initHints() : %d - ai_socktype, %d - ai_flags\n\n", hints.ai_socktype, hints.ai_flags);
    // printf("\n After initSigHandler() : %d\n\n", sa.sa_flags);

    // Load address structs with getaddrinfo()
    if((status = getaddrinfo(0, serverCmdOptions.port, &hints, &servInfo) != 0)) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    // Create and bind socket
    for(curr = servInfo; curr != 0; curr = curr->ai_next) {
        // create the socket
        if((serverSocket = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol)) == -1) {
            perror("Server - Socket failure");
            continue; // keep trying
        }

        // bind it
        if(bind(serverSocket, curr->ai_addr, curr->ai_addrlen) == -1) {
            close(serverSocket);
            perror("Server - Bind failure");
            continue; // keep trying
        }
        break; // quit looping on success
    }

    // Quit if socket did not bind correctly
    if(curr == 0) {
        fprintf(stderr, "Server - Bind failure\n");
        freeaddrinfo(servInfo);
        exit(1);
    }

    //
    // TCP PROTOCOL
    //
    if(hints.ai_socktype == SOCK_STREAM) {
        // Reuse socket address by passing socket option SO_REUSEADDR
        if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("Server - Failure of setsockopt()");
            close(serverSocket);
            freeaddrinfo(servInfo);
            exit(1);
        }

        while(1) {
            printf("****************************************************************************\n");
            printf("Server - Listening...\n");
            // Server listens for connections
            if(listen(serverSocket, BACKLOG) == -1) {
                perror("Server - Error with listen()");
                close(serverSocket);
                freeaddrinfo(servInfo);
                exit(1);
            }

            // Server accepts a connection
            if((conn = accept(serverSocket, (struct sockaddr*)&addr, &addrLen)) == -1) {
                perror("Server - Error with accept()");
                close(serverSocket);
                close(conn);
                freeaddrinfo(servInfo);
                exit(1);
            }

            // Convert address from network byte order
            inet_ntop(addr.sin_family, getSocketIPAddress((struct sockaddr*)&addr), addrStr, sizeof(addrStr));
            printf("Server - Connected via TCP to %s on port %s\n\n", addrStr, serverCmdOptions.port);

            // Receive a packet
            if((numberBytes = recv(conn, buf, sizeof(struct packet), 0)) == -1) {
                perror("Server - recv");
                close(serverSocket);
                close(conn);
                freeaddrinfo(servInfo);
                exit(1);
            }
            printf("Server - %d bytes received\n", numberBytes);

            // Convert packet from network byte order
            uint32_t data = ntohl(((struct packet*)buf)->number);
            printf("Version - %d\n", ((struct packet*)buf)->version);
            printf("Server - Received data : %u\n", data);

            // Fork child
            // Send reply to client
            printf("Server - Responding to the client...\n");
            if(!fork()) {
                if((sendall(conn, (char*)&serverReply, &numberBytes)) == -1) {
                    fprintf(stderr, "Server - Failure with sendall()\n");
                }
                close(serverSocket);
                close(conn);
                freeaddrinfo(servInfo);
                exit(0);
            }

            // Server closes connection, keeps listening
            close(conn);
        }
    }

    //
    // UDP Protocol
    //
    if(hints.ai_socktype == SOCK_DGRAM) {
        while(1) {
            // Server waits to receive a datagram
            printf("****************************************************************************\n");
            printf("Server - Waiting for datagram\n");
            if((numberBytes = recvfrom(serverSocket, buf, sizeof(struct packet), 0, (struct sockaddr*)&addr2, &addr2Len)) == -1) {
                perror("Server - Failure with recvfrom()");
                close(serverSocket);
                freeaddrinfo(servInfo);
                exit(1);
            }

            // Server receives datagram
            printf("Server - Received %d bytes from %s:%s via UDP protocol\n", numberBytes, inet_ntoa(addr2.sin_addr), serverCmdOptions.port);

            // Convert from network byte order
            uint32_t data = ntohl(((struct packet*)buf)->number);
            printf("Server - The sent number is: %d", data);

            // Server replies to the client
            printf("\nServer - Replying to the client...\n");
            if((sendto(serverSocket, &serverReply, sizeof(serverReply), 0, (struct sockaddr*)&addr2, addr2Len)) == -1) {
                perror("Server - Failure with sendto()");
                close(serverSocket);
                freeaddrinfo(servInfo);
                exit(1);
            }
        }
    }
    exit(0);
}


///////////////////////////////////////////////////////////////////////////////////////////////
//
// HELPER FUNCTIONS
//
///////////////////////////////////////////////////////////////////////////////////////////////
struct cmdline parse(int argc, char* argv[]) {
    struct cmdline serverCmdOptions;
    int option = 0;
    int numberOfOptions = 0;

    while((option = getopt(argc, argv, "t: p:")) != -1) {
        switch(option) {
            // type of socket requested
            case 't':
                strncpy(serverCmdOptions.socketType, optarg, 3);
                numberOfOptions++;
                break;
            // which port number to use
            case 'p':
                serverCmdOptions.port = optarg;
                numberOfOptions++;
                break;
            default:
                usage();
        }
    }

    if(numberOfOptions != 2) {
        printf("Please enter the correct number of parameters\n");
        usage();
    }

    return serverCmdOptions;
}


void initHints(struct addrinfo* hints, char* socketType) {
    memset(hints, 0, sizeof(*hints));
    hints->ai_family = AF_INET; // IPv4
    hints->ai_flags = AI_PASSIVE; // Assign the address of my local host to the socket
    if(strncmp(socketType, "tcp", 3)==0) {
        hints->ai_socktype = SOCK_STREAM; // TCP
    } 
    else if(strncmp(socketType, "udp", 3)==0) {
        hints->ai_socktype = SOCK_DGRAM; // UDP
    }
}


void initSigHandler(struct sigaction* sa) {
    sa->sa_handler = sigchld_handler;
    sigemptyset(&sa->sa_mask);
    sa->sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, sa, 0)== -1) {
        perror("sigaction");
        exit(1);
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
    return n==-1?-1:0; // return -1 on failure, 0 on success
}


void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it later
    int saved_errno = errno;
    int status;

    while(waitpid(-1, &status, WNOHANG) > 0) {
        if(WIFEXITED(status)) {
            printf("Success - Server request completed. Child terminated normally\n");
        } 
        else {
            perror("Error - Server request. Child terminated abnormally\n");
        }
    }
    // restore errno
    errno = saved_errno;
}

void usage() {
    printf("\nUsage - <-Option> <Value>\n\n");
    printf("\t-t <tcp> or <udp> to specify protocol\n\n");
    printf("\t-p <port> number\n");
    exit(0);
}