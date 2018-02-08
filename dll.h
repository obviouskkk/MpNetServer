#ifndef __DLL_H__
#define __Dll_H__

#include <liblog/log.h>
#include <sys/types.h>
#include "service.h" 

typedef struct ServInterface {
	void*   handle; // Hold the handle returned by dlopen
	int		(*get_pkg_len)(int fd, const void* avail_data, int avail_len, int isparent);
	int		(*proc_pkg_from_client)(void* pkg, int pkglen, const fdsession_t* fdsess);
	void	(*proc_pkg_from_serv)(int fd, void* pkg, int pkglen);
	void	(*on_client_conn_closed)(int fd);
	void	(*on_fd_closed)(int fd);
	int 	(*init_service)(int isparent);
	int 	(*fini_service)(int isparent);
	int		(*proc_udp_pkg)(int fd, const void* avail_data, int avail_len ,struct sockaddr_in * from, socklen_t fromlen );
	void	(*proc_mcast_pkg)(const void* data, int len);
} serv_if_t;

extern serv_if_t dll;

static inline uint32_t get_server_id()
{
	return config_cache.bc_elem->server_id;
}

static inline const char* get_server_ip()
{
	return config_cache.bc_elem->bind_ip;
}


static inline in_port_t get_server_port()
{
	return config_cache.bc_elem->bind_port;
}


static inline const char* get_server_name()
{
	return config_cache.bc_elem->server_name;
}


static inline uint32_t get_cli_ip(const fdsession_t* fdsess)
{
	return fdsess->remote_ip;
}

static inline uint32_t get_cli_port(const fdsession_t* fdsess)
{
	return fdsess->remote_port;
}


int  register_plugin(const char* file_name);
void unregister_plugin();









#endif
