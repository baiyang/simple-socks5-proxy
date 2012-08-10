#include "socks.h"

Socket _socks_server;

void do_listen()
{
	msg("Listening on port 1080");
	
	Socket _client;
	RequestVersion req_versoin;
	unsigned int size = sizeof(_client._addr_info);
	
	while (true){
		_client._sock = accept(_socks_server._sock, (sockaddr*)&_client._addr_info, &size);
		if(_client._sock == -1){
			continue;
		}
		
		if (confirm_request(_client, req_versoin) == false){
			msg("Confirm failure");
			continue;
		}
		
		msg("Confirm success");
		pthread_t pid;
		
		pthread_create(&pid, NULL, build_connection, (void*)&_client);
	}
	
}


bool confirm_request(Socket _sock, RequestVersion &req_versoin)
{

	char buff[1024];
	int len;
	
	char *ip = inet_ntoa(_sock._addr_info.sin_addr);
	int port = _sock._addr_info.sin_port;
	
	char str_msg[1024] = "Confirm ";
	len = sprintf(str_msg, "Confirm %s on port %d", ip, ntohs(port) );
	str_msg[len] = '\0';
	msg(str_msg);
	
	if ( (len = recv(_sock._sock, buff, 1024, 0)) == -1){
		return false;
	}
	
	buff[len] = '\0';
	req_versoin = *((RequestVersion*)buff);
	if (req_versoin._ver != 0x05){
		return false;
	}
	
	/*** Only handle un-authentication required ***/
	ReplyVersion reply_version;
	reply_version._cmd = 0x00;
	reply_version._ver = 0x05;
	strcpy(buff, (char*)&reply_version);
	
	if ((len = send(_sock._sock, buff, 2, 0)) == -1){
		return false;
	}
	
	return true;
}


void* build_connection(void *data)
{
	
	Socket _sock = *(Socket*)data;
	char buff[1024];
	int len;
	RequestBuild req_build;
	ReplyBuild   reply_build;
	
	if ( (len = recv(_sock._sock, buff, 1024, 0)) == -1){
		close(_sock._sock);
		return 0;
	}
	
	buff[len] = '\0';
	req_build = *(RequestBuild*)buff;
	reply_build = *(ReplyBuild*)buff;
	
	/*** reply msg to client ***/
	unsigned int size = sizeof(sockaddr);
	reply_build._cmd = 0x00;
	
	/*** Only handle Connect Command ***/
	if( req_build._cmd != 0x01){
		reply_build._cmd = 0x07;
		msg("Only handle Connect Command");
	}
	
	/*** handle address and port ***/
	Socket dst_socket;
	if (resolve_addr_and_port(req_build, dst_socket._addr_info) == false){
		reply_build._cmd = 0x04;
		msg("Error in resolving ");
	}
	
	dst_socket._sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
	size = sizeof(sockaddr_in);
	if (connect(dst_socket._sock, (sockaddr*)&dst_socket._addr_info, size) == -1){
		reply_build._cmd = 0x03;
		msg("Unreachable Host");
	}
	
	/*** failure ***/
	if(reply_build._cmd != 0x00){
		strcpy(buff, (char*)&reply_build);
		size = strlen(buff);
		send(_sock._sock, buff, size, 0);
		close(_sock._sock);
		return 0;
	}
	
	strcpy(buff, (char*)&reply_build);
	len =send(_sock._sock, buff, len, 0);
	if( len <= 0){
		close(_sock._sock);
		return 0;
	}

	/*** build connection ***/
	Connection conn;
	conn._dst = dst_socket;
	conn._src = _sock;
	do_select(conn);
}


bool resolve_addr_and_port(RequestBuild& req_build, sockaddr_in& addr)
{
	int len;
	char domain[256];
	hostent *host;
	char **pp;
	
	addr.sin_family = AF_INET;
	
	switch(req_build._atyp){
		case 0x04:
			msg("Don't Support Ipv6");
			return false;
		break;
		
		case 0x01: // handle ipv4
 			addr.sin_port = *(unsigned short*)req_build.AddrPort._port;
			addr.sin_addr = *(in_addr*)req_build.AddrPort._ipv4;
		break;
		
		case 0x03: // handle domain
			len = int(req_build.AddrPort._info[0]);
			strncpy(domain, req_build.AddrPort._info + 1, len);
			addr.sin_port = *(unsigned short*)(req_build.AddrPort._info + len + 1);
			domain[len] = '\0';
			
			host = gethostbyname(domain);
			if(host == NULL){
				msg("error in gethostbyname");
				return false;
			}
			/*** Only support ipv4 ***/
			if( host->h_addrtype != AF_INET){
				return false;
			}
			
			pp = host->h_addr_list;
			addr.sin_addr = *(in_addr*)(*pp);
			
		break;
		
		default:
			msg("Unknown address type");
			return false;
	}
	
	return true;
}

void msg(char* msg)
{
	std::cout<<msg<<std::endl;
}


void do_select(Connection &conn)
{	
	int ret, len;
	int max_fd;
	char buff[4096];
	fd_set read_set;
	
	while(true){
		FD_ZERO(&read_set);
		
		max_fd = 0;
		max_fd = conn._src._sock > max_fd ? conn._src._sock : max_fd;
		max_fd = conn._dst._sock > max_fd ? conn._dst._sock : max_fd;
		
		FD_SET(conn._src._sock, &read_set);
		FD_SET(conn._dst._sock, &read_set);
		
		/*** Block until some fds are ready to write or read ***/
		ret = select(max_fd + 1, &read_set, NULL, NULL, 0);
		if( ret == -1){
			msg("select error");
			break;
		}
		if(ret > 0){
			if( FD_ISSET(conn._src._sock, &read_set) ){
				len = recv(conn._src._sock, buff, 4096, 0);
				if( len <= 0){
					msg("src error");
					break;
				}
				send(conn._dst._sock, buff, len, 0);
				buff[len] = '\0';
				msg(buff);
			}
			
			if( FD_ISSET(conn._dst._sock, &read_set) ){
				len = recv(conn._dst._sock, buff, 4096, 0);
				if( len <= 0){
					break;
				}
				send(conn._src._sock, buff, len, 0);
				buff[len] = '\0';
				msg(buff);
			}
		}
	}
	
	msg("out of select loop");
}


