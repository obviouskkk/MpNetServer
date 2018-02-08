#ifndef __NET_H__
#define __NET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include "List.h"
#include "bindconf.h"

enum {
	//page_size	= 8192
	def_page_size = 8192
};

#define CN_NEED_CLOSE	0x01
#define CN_NEED_POLLIN	0x02

struct bind_config_info;

typedef struct conn_buf {
	uint32_t	rcvprotlen; //协议要接收的长度
	uint32_t	recvlen; //接收到的长度
	uint32_t	sendlen; //发送的长度
	uint32_t	sndbufsz; //缓冲区剩余大小
	uint8_t*	recvptr; //接收包的指针
	uint8_t*	sendptr; //发送包指针
} conn_buf_t;

typedef struct remote_info {
	uint16_t	remote_port; //远端端口
	uint32_t	remote_ip; //远端 IP
	uint32_t	last_tm; //上次交互的时间
} __attribute__((packed)) remote_info_t;

enum {
	fd_type_unused = 0,//已经关闭的端口；
	fd_type_listen, //
	fd_type_pipe,
	fd_type_remote,
	fd_type_mcast,
	fd_type_addr_mcast,
	fd_type_udp,
	fd_type_asyn_connect
};

typedef struct fdinfo {
	uint32_t	id;
	int			sockfd;
	uint8_t		type;
	uint8_t		flag;
	conn_buf_t	cb;
	remote_info_t	sk;
	struct bind_config_info*	bc_elem;
	void		(*callback)(int fd, void* arg);
	void*		arg;
	list_t	list;
} fdinfo_t;


struct epinfo {
	fdinfo_t*	fds;
	struct epoll_event*	evs;
	list_t	close_head;
	list_t	etin_head;
	int			epfd;
	int			maxfd;
	int			max_ev_num;
	int			count;
} ;

extern struct epinfo epi;
extern time_t socket_timeout;
extern int page_size;
extern int listen_port;
extern char  listen_ip[16];
extern uint32_t	send_buf_limit_size;

int  net_init(int size, int maxevents);
int  net_loop(int timeout, int max_len, int is_conn);
int  net_start(const char *listen_ip, uint16_t listen_port, struct bind_config_info* bc_elem);
void net_exit();

int  mod_events(int epfd, int fd, uint32_t flag);
int  do_write_conn(int fd);
int  do_add_conn(int fd, uint8_t type, struct sockaddr_in *peer, struct bind_config_info* bc_elem);
void do_del_conn(int fd, int is_conn);







#ifdef __cplusplus
} // end of extern "C"
#endif



#endif
