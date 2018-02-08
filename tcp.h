#ifndef __TCP_H__
#define __TCP_H__

#include <netinet/in.h>


int set_io_blockability(int fd, int nonblock);


int set_sock_snd_timeo(int sockfd, int millisec);

int set_sock_rcv_timeo(int sockfd, int millisec);

int safe_tcp_accept(int sockfd, struct sockaddr_in* peer, int nonblock);

int safe_tcp_connect(const char* ipaddr, in_port_t port, int timeout, int nonblock);


int safe_socket_listen(const char* ipaddr, in_port_t port, int type, int backlog, int bufsize);

int create_passive_endpoint(const char* host, const char* serv, int socktype, int backlog, int bufsize);

int safe_tcp_send_n(int sockfd, const void* buf, int total);


int safe_tcp_recv(int sockfd, void* buf, int bufsize);

int safe_tcp_recv_n(int sockfd, void* buf, int total);



#endif
