#ifndef _SOCKS__H
#define _SOCKS__H

#include "util.h"
#include <vector>
#include <iostream>
#include <cstring>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sstream>


const int MAX_CLIENTS = 10;
const int PORT = 1080;


void msg(char *msg);

bool confirm_request(Socket _sock, RequestVersion &req_version);
bool resolve_addr_and_port(RequestBuild &req_build, sockaddr_in &addr);
void do_listen();
void do_select(Connection &);

/*** thread function ***/
void* build_connection(void *data);


#endif  //endif