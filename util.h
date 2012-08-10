#ifndef _UTIL__H
#define _UTIL__H

#include <sys/socket.h>
#include <netinet/in.h>

/*** 
 * packet: from client to server
 * purpos: send a version identifier/method selection message
 * ***/
#pragma pack(push, 1)

typedef struct {
	char _ver;
	char _nmethods;
	char *_methods;
}RequestVersion;

#pragma pack(pop)



/***
 * server selects a command 
 * ***/
#pragma pack(push, 1)

typedef struct {
	char _ver;
	char _cmd;
}ReplyVersion;

#pragma pack(pop)



/***
 * it only handles ipv4 request
 * ***/
#pragma pack(push, 1)

typedef struct {
	char _ver;
	char _cmd;
	char _rsv;
	char _atyp;
	union {
		char _info[1024]; // domain + port
		char _ipv4[4];
		char _port[2];
	}AddrPort;
}RequestBuild, ReplyBuild;

#pragma pack(pop)


/***
 * Socket packet 
 * ***/
typedef struct {
	int _sock;
	sockaddr_in _addr_info;
}Socket;
 
 
 /***
  * socket Connection
  * ***/ 
typedef struct {
	Socket _src;
	Socket _dst;
}Connection;

#endif // endif
