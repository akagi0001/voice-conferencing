#ifndef STATUS_H
#define	STATUS_H

#include <netdb.h>

#include "../constants.h"

typedef struct {
    char connected_server_ip[MAXBUFSIZE];
    int connected_server_port;
    
    int sockfd;
    struct addrinfo *p;
    
    int voicefd;
    struct addrinfo *udp;
    
    char client_id[MAXBUFSIZE];
    char password[MAXBUFSIZE];
    
    int current_session_id;
    
} Status;

Status status;

#endif	/* STATUS_H */

