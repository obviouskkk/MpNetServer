#ifndef __NET_IF_H__
#define __NET_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "service.h"

typedef struct ip_port {
	/*! ip */
	char		ip[16];
	/*! port */
	in_addr_t	port;
} ip_port_t;


int connect_to_svr(const char* ipaddr, in_addr_t port, int bufsz, int timeout);


int connect_to_service(const char* service_name, uint32_t svr_id, int bufsz, int timeout);


//int asyn_connect_to_svr(const char* ipaddr, in_addr_t port, int bufsz, void (*callback)(int fd, void* arg), void* arg);

int asyn_connect_to_service(const char* service_name, uint32_t svr_id, int bufsz, void (*callback)(int fd, void* arg), void* arg);


void close_svr(int svrfd);

int create_udp_socket(struct sockaddr_in* addr, const char* ip, in_port_t port);


const char* resolve_service_name(const char* service_name, uint32_t svr_id);

const ip_port_t* get_last_connecting_service();

static inline
uint32_t get_remote_ip(int fd)
{
	if ((fd >= 0) && (fd <= epi.maxfd) && (epi.fds[fd].type != fd_type_unused)) {
		return epi.fds[fd].sk.remote_ip;
	}

	return 0;
}


int net_send(int fd, const void* data, uint32_t len);

int send_pkg_to_client(const fdsession_t* fdsess, const void* pkg, const int pkglen);


void close_client_conn(int fd);


#ifdef __cplusplus
} // end of extern "C"
#endif




#endif
