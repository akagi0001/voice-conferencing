#ifndef CLIENT_H
#define	CLIENT_H

#include "../constants.h"
#include "../message.h"
#include "../packet_type.h"

void login(int client_id, char* password, char* server_ip, int server_port);

void logout();

void join_session(int session_id);

void leave_session();

void create_session(int session_id);

void list();

void quit();

void send_message(char* message);

#endif	/* CLIENT_H */
