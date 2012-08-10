#include "work.h"

bool init()
{
	int port = PORT;
	if( (_socks_server._sock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		msg("error in socket");
		return false;
	}
	
	_socks_server._addr_info.sin_port = htons(port);
	_socks_server._addr_info.sin_addr.s_addr = inet_addr("127.0.0.1");
	_socks_server._addr_info.sin_family = AF_INET;
	if( bind(_socks_server._sock, (sockaddr*)&_socks_server._addr_info, sizeof(sockaddr) ) == -1){
		msg("error in bind");
		return false;
	}
	
	if( listen(_socks_server._sock, MAX_CLIENTS) == -1){
		msg("error in listen");
		return false;
	}
	
	return true;
}

void serve_forever()
{
	bool ret;
	ret = init();
	if(!ret){
		msg("error in serve_forever");
		return;
	}
	
	do_listen();
	shutdown();
}

void shutdown()
{
	close(_socks_server._sock);
	
}
