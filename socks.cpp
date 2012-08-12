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
			close(_client._sock);
			continue;
		}
		
		msg("Confirm success");
		pthread_t pid;
		pthread_create(&pid, NULL, build_connection, (void*)&_client);
	}
}


bool confirm_request(Socket _sock, RequestVersion &req_versoin)
{
	int ret;
	
	ret = sock_read(_sock._sock, req_versoin);
	if( ret <= 0){
		return false;
	}

	/*** Only handle un-authentication required ***/
	ReplyVersion reply_version;
	reply_version._cmd = 0x00;
	reply_version._ver = 0x05;
	
	ret = sock_write(_sock._sock, reply_version, 2);
	if( ret <=0 ){
		return false;
	}
	
	return true;
}


void* build_connection(void *data)
{

	Socket _sock = *(Socket*)data;
	int packet_len;
	RequestBuild req_build;
	ReplyBuild   reply_build;
	
	packet_len = sock_read(_sock._sock, req_build);
	if( packet_len <= 0){
		close(_sock._sock);
		return 0;
	}
	reply_build = *(ReplyBuild*)&req_build;

	/*** reply msg to client ***/
	unsigned int size = sizeof(sockaddr);
	reply_build._cmd = 0x00;
	
	/*** Only handle Connect Command ***/
	if( req_build._cmd != 0x01){
		reply_build._cmd = 0x07;
		sock_write(_sock._sock, reply_build, packet_len);
		close(_sock._sock);
		return 0;
	}
	
	/*** resolve address and port ***/
	Socket dst_socket;
	dst_socket._sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
	
	if (resolve_addr_and_port(req_build, dst_socket._addr_info) == false){
		reply_build._cmd = 0x04;
		msg("Error in resolving ");
		sock_write(_sock._sock, reply_build, packet_len);
		close(_sock._sock);
		return 0;
	}
	
	size = sizeof(sockaddr_in);
	if (connect(dst_socket._sock, (sockaddr*)&dst_socket._addr_info, size) == -1){
		reply_build._cmd = 0x03;
		msg("Unreachable Host");
		sock_write(_sock._sock, reply_build, packet_len);
		close(_sock._sock);
		return 0;
	}

	sock_write(_sock._sock, reply_build, packet_len);

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
		case IPV6:
			msg("Don't Support Ipv6");
			return false;
			break;
		case IPV4:
 			addr.sin_port = *(unsigned short*)req_build.AddrPort._port;
			addr.sin_addr = *(in_addr*)req_build.AddrPort._ipv4;
			break;
		case DOMAIN:
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
					msg("dst error");
					break;
				}
				send(conn._src._sock, buff, len, 0);
				buff[len] = '\0';
			}
		}
	}

	/*** close sockets on this connection***/
	close(conn._src._sock);
	close(conn._dst._sock);
}

template <class T>
int sock_read(int& sock, T& data)
{
	char buff[1024];
	int  len;
	
	len = recv(sock, buff, 1024, 0);
	if (len <= 0){
		return len;
	}
	
	buff[len] = '\0';
	data = *(T*)buff;
	return len;
}

template <class T>
int  sock_write(int& sock, T& data, int left_len = -1)
{
	int sent_len = 0;
	int temp = 0;

	while( left_len > 0){
		temp = send(sock, ((char*)&data) + (sent_len += temp), left_len -= temp, 0);
		if(temp <= 0){
			break;
		}
	}
	
	return sent_len;
}
