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


enum ADDR_TYPE{IPV6 = 0x04, IPV4 = 0x01, DOMAIN = 0x03};

const int MAX_CLIENTS = 10;
const int PORT = 1080;

template <class T>
int sock_read(int &, T &);

template <class T>
int sock_write(int &, T &, int = -1);


void msg(char *msg);

bool confirm_request(Socket _sock, RequestVersion &req_version);
bool resolve_addr_and_port(RequestBuild &req_build, sockaddr_in &addr);
void do_listen();
void do_select(Connection &);

/*** thread function ***/
void* build_connection(void *data);

#endif  //endif