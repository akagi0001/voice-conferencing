#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <assert.h>

#include "server.h"

int port;
fd_set master; // master file descriptor list

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

void usage(char *program) {
    fprintf(stderr, "Usage: %s <TCP port number to listen on>\n", program);
    exit(1);
}

void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void open_server_socket(int port) {
    char server_port_s[PORT_BUF_SIZE];
    snprintf(server_port_s, PORT_BUF_SIZE, "%d", port);
    int new_fd; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, server_port_s, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof (int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    // Clear all dead processes
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

}

struct timeval tv = {
    .tv_sec = 1,
    .tv_usec = SERVER_NACK_TIMEOUT,
};

void listen_for_messages() {
    //Initialize File Descriptor
    fd_set read_fds; // temp file descriptor list for select()
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &master); // add the listener to the master set

    int fdmax = sockfd; // keep track of the biggest file descriptor

    struct sockaddr_storage remoteaddr; // client address
    char remoteIP[INET6_ADDRSTRLEN];

    int nbytes;

    // main loop
    PRINT("Waiting for Clients\n");

    while (1) {
        // Copy the master to the temporary FD
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, &tv) == -1) {

            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!

                // New Connections
                if (i == sockfd) {
                    socklen_t addrlen = sizeof remoteaddr;
                    int newfd = accept(sockfd,
                            (struct sockaddr *) &remoteaddr, &addrlen);
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) fdmax = newfd;

                        PRINT("New Con. %s on socket %d\n",
                                inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*) &remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN
                                ),
                                newfd
                                );
                    }
                } else {
                    Message* msg = (Message*) malloc(sizeof (Message));
                    if ((nbytes = recv(i, msg, sizeof (Message), 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) PRINT("selectserver: socket %d hung up\n", i);
                        else perror("recv");

                        FD_CLR(i, &master); // remove from master set
                        close(i); // close the stream

                    } else {
                        if (DEBUG_MSG) print_message(msg);
                        handle_client_message(msg, i);
                    }
                    free(msg);
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END while(1) 
}

int sockfd;
AddrInfo *p;

int main(int argc, char *argv[]) {
    if (argc != 2) usage(argv[0]);
    PRINT("Started\n");

    // Obtain the Port Number
    port = atoi(argv[1]);
    if (!(MIN_PORTNUM <= port && port <= MAX_PORTNUM)) {
        fprintf(stderr, "port = %d should be within range [%d:%d]\n", port, MIN_PORTNUM, MAX_PORTNUM);
        usage(argv[0]);
    }

    // Open a server side socket
    open_server_socket(port);

    // The main iteration loop
    listen_for_messages();

    close(sockfd);
    return 0;
}
