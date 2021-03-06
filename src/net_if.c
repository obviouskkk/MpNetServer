#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "timer.h"
#include "tcp.h"
#include "net.h"
#include "service.h"
#include "net_if.h"
#include "log.h"


static int g_ip_resolved;
static ip_port_t g_ip_port;

int connect_to_svr(const char* ipaddr, in_addr_t port, int bufsz, int timeout)
{
	struct sockaddr_in peer;
	int fd;

	memset(&peer, 0, sizeof(peer));
	peer.sin_family  = AF_INET;
	peer.sin_port    = htons(port);
	if (inet_pton(AF_INET, ipaddr, &peer.sin_addr) <= 0) {
		printf("inet_pton %s failed", ipaddr);
	}

	fd = safe_tcp_connect(ipaddr, port, timeout, 1);
	if (fd != -1) {
		DEBUG_LOG("CONNECTED TO\t[%s:%u fd=%d]", ipaddr, port, fd);
		do_add_conn(fd, fd_type_remote, &peer, 0);
	} else {
		ERROR_LOG("failed to connect to %s:%u, err=%d %s", ipaddr, port, errno, strerror(errno));
	}

	return fd;
}

/*
int asyn_connect_to_svr(const char* ipaddr, in_addr_t port, int bufsz, void (*callback)(int fd, void* arg), void* arg)
{
	struct sockaddr_in peer;

	memset(&peer, 0, sizeof(peer));
	peer.sin_family  = AF_INET;
	peer.sin_port    = htons(port);
	if (inet_pton(AF_INET, ipaddr, &peer.sin_addr) <= 0) {
		ERROR_LOG("failed to connect to %s:%u, err=%d (inet_pton: %s)",ipaddr, port, errno, strerror(errno));
		return -1;
	}

	// create socket
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		ERROR_LOG("failed to connect to %s:%u, err=%d (socket: %s)",ipaddr, port, errno, strerror(errno));
		return -1;
	}
	set_io_blockability(sockfd, 1);

	// connect to svr
	int done = 1;
connect_again:
	if (connect(sockfd, (void*)&peer, sizeof(peer)) < 0) {
		switch (errno) {
		case EINPROGRESS:
			done = 0;
			break;
		case EINTR:
			goto connect_again;
		default:
			ERROR_LOG("failed to connect to %s:%u, err=%d (connect: %s)",ipaddr, port, errno, strerror(errno));
			close(sockfd);
			return -1;
		}
	}

	assert(done == 0);
//	if (done) {
// try best to avoid calling 'callback' inside function asyn_connect_to_svr 
//		DEBUG_LOG("CONNECTED TO\t[%s:%u fd=%d]", ipaddr, port, sockfd);
//		do_add_conn(sockfd, fd_type_remote, &peer, 0);
//		callback(sockfd, arg);
//		return 1; // if we could avoding calling 'callback' here, the return value is always 0 or -1
//	}

	do_add_conn(sockfd, fd_type_asyn_connect, &peer, 0);	
	epi.fds[sockfd].callback = callback;
	epi.fds[sockfd].arg 	 = arg;
	printf("ASYNC CONNECT TO[fd=%d id=%u]\n", sockfd, epi.fds[sockfd].id);

	return 0;
}
*/ 

void close_svr(int svrfd)
{
	do_del_conn(svrfd, 2);
}

int create_udp_socket(struct sockaddr_in* addr, const char* ip, in_port_t port)
{
	memset(addr, 0, sizeof(*addr));

	addr->sin_family = AF_INET;
	addr->sin_port   = htons(port);
	if ( inet_pton(AF_INET, ip, &(addr->sin_addr)) <= 0 ) {
		return -1;
	}

	return socket(PF_INET, SOCK_DGRAM, 0);
}

int net_send(int fd, const void* data, uint32_t len)
{
	int prev_stat = 0;
	int send_bytes;
	//tcp linger send
	if (epi.fds[fd].cb.sendlen > 0) {
		if (do_write_conn(fd) == -1) {
			do_del_conn(fd, is_parent);
			return -1;
		}
		prev_stat = 1;
	}

	send_bytes = 0;
	if (epi.fds[fd].cb.sendlen == 0) {
		send_bytes = safe_tcp_send_n(fd, data, len);
		if (send_bytes == -1) {
			ERROR_LOG("failed to write to fd=%d err=%d %s", fd, errno, strerror(errno));
			do_del_conn(fd, is_parent);
			return -1;
		}
	}

	//merge buffer
    //EAGAIN 没发送完，只发送了send_bytes字节
	if (len > send_bytes){
		if (!epi.fds[fd].cb.sendptr) {
			epi.fds[fd].cb.sendptr = (uint8_t*) malloc (len - send_bytes);
			if (!epi.fds[fd].cb.sendptr) {
				ERROR_LOG("malloc error, %s", strerror(errno) );
                exit(1);
            }
			epi.fds[fd].cb.sndbufsz = len - send_bytes;
			
		} else if (epi.fds[fd].cb.sndbufsz < epi.fds[fd].cb.sendlen + len - send_bytes) {
			epi.fds[fd].cb.sendptr = (uint8_t*)realloc (epi.fds[fd].cb.sendptr,
					epi.fds[fd].cb.sendlen + len - send_bytes);
			if (!epi.fds[fd].cb.sendptr) {
				ERROR_LOG("realloc error, %s", strerror(errno));
                exit(1);
            }
            
			epi.fds[fd].cb.sndbufsz = epi.fds[fd].cb.sendlen + len - send_bytes;
		}
			
		memcpy(epi.fds[fd].cb.sendptr + epi.fds[fd].cb.sendlen, (char*)data + send_bytes, len - send_bytes);
		epi.fds[fd].cb.sendlen += len - send_bytes;//sendlen+=剩余没发送完长度
		if (is_parent && (send_buf_limit_size > 0)
				&& (epi.fds[fd].cb.sendlen > send_buf_limit_size)) {
			ERROR_LOG("send buf limit exceeded: fd=%d buflen=%u limit=%u",
						fd, epi.fds[fd].cb.sendlen, send_buf_limit_size);
			return -1;
		}
	}
    
	if (epi.fds[fd].cb.sendlen > 0 && !prev_stat) 
		mod_events (epi.epfd, fd, EPOLLOUT | EPOLLIN);
	else if (prev_stat && epi.fds[fd].cb.sendlen == 0)
		mod_events (epi.epfd, fd, EPOLLIN);

	return 0;
}

int send_pkg_to_client(const fdsession_t* fdsess, const void* pkg, const int pkglen)
{
	shm_block_t mb;

	mb.id   = fdsess->id;
	mb.fd   = fdsess->fd;
	mb.type = DATA_BLOCK;
	
    int send_bytes, cur_len;
	for (send_bytes = 0; send_bytes < pkglen; send_bytes += cur_len) {
		if ((pkglen - send_bytes) > (page_size - sizeof(shm_block_t))) {
			cur_len = page_size - sizeof(shm_block_t);
		} else {
			cur_len = pkglen - send_bytes;
		}

		mb.length = cur_len + sizeof(shm_block_t);

		if (shmq_push(&(config_cache.bc_elem->sendq), &mb, (char*)pkg + send_bytes) == -1) {
			return -1;
		}
	}

	return 0;	
}

void close_client_conn(int fd)
{
	shm_block_t mb;

	const fdsession_t* fdsess = get_fdsess(fd);
	if (!fdsess) {
		return;
	}

	mb.id     = fdsess->id;
	mb.length = sizeof(shm_block_t);
	mb.type   = FIN_BLOCK;
	mb.fd     = fd;

	handle_close(fd);
	shmq_push(&(config_cache.bc_elem->sendq), &mb, 0);
}


