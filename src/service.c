#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include "config.h"
#include "service.h"
#include "shmq.h"
#include "net.h"
#include "dll.h"
#include "net_if.h"
#include "log.h"

int 	is_parent = 1;
fd_array_session_t	fds;
config_cache_t		config_cache;

static inline void add_fdsess(fdsession_t* fdsess)
{
	g_hash_table_insert(fds.cn, &(fdsess->fd), fdsess);
	++(fds.count);
}

static inline void remove_fdsess(int fd)
{
	g_hash_table_remove(fds.cn, &fd);
	--(fds.count);
}

static inline void free_fdsess(void* fdsess)
{
	g_slice_free1(sizeof(fdsession_t), fdsess);
}

static inline int handle_init(bind_config_info_t* bc_elem)
{
	config_cache.idle_timeout = config_get_intval("idle_timeout", 10);
	config_cache.bc_elem      = bc_elem;

	fds.cn = g_hash_table_new_full(g_int_hash, g_int_equal, 0, free_fdsess);
	
	return (dll.init_service ? dll.init_service(0) : 0);	
}

static inline int handle_fini()
{
	int i;
	for (i = 0; i <= epi.maxfd; ++i) {
		if ( (epi.fds[i].type == fd_type_remote) && (epi.fds[i].cb.sendlen > 0) ) {
			return 0;
		}
	}
	if ( dll.fini_service && (dll.fini_service(0) != 0) ) {
		return 0;
	}
	g_hash_table_destroy(fds.cn);
	return 1;
}

static inline int handle_open(const shm_block_t* mb)
{
	const fdsession_t* fdsess = get_fdsess(mb->fd);
	if (fdsess || (mb->length != (sizeof(shm_block_t) + sizeof(remote_info_t )))) {
		printf("handle_open OPEN_BLOCK, fd=%d length=%d", mb->fd, mb->length);
		return -1;
	} else {
		fdsession_t* fdsess  = g_slice_alloc(sizeof *fdsess);
		fdsess->fd           = mb->fd;
		fdsess->id           = mb->id;
		fdsess->remote_port  = *(uint16_t*)mb->data;
		fdsess->remote_ip    = *(uint32_t*)&mb->data[2];
		add_fdsess(fdsess);
	}
	return 0;
}

int handle_close(int fd)
{
	const fdsession_t* fdsess = get_fdsess(fd);
	if (!fdsess) {
		ERROR_LOG("connection %d had already been closed", fd );
	}
	//assert(fds.count > 0);
	dll.on_client_conn_closed(fd);
	remove_fdsess(fd);
	return 0;
}

static inline void handle_process(uint8_t* recvbuf, int rcvlen, int fd)
{
	const fdsession_t* fdsess = get_fdsess(fd);
	if (fdsess) {
		if (dll.proc_pkg_from_client(recvbuf, rcvlen, fdsess)) {
			close_client_conn(fd);
		}
	}
}

void handle_recv_queue()
{
	struct shm_queue* recvq = &(config_cache.bc_elem->recvq);
	struct shm_queue* sendq = &(config_cache.bc_elem->sendq);

	struct shm_block* mb;
	while (shmq_pop(recvq, &mb) == 0) {
		switch (mb->type) {
		case DATA_BLOCK:
			if ( mb->length > sizeof(*mb) ) {
				handle_process(mb->data, mb->length - sizeof(*mb), mb->fd);
			}
			break;
		case OPEN_BLOCK:
			if ( handle_open(mb) == -1 ) {
				mb->type    = FIN_BLOCK;
				mb->length  = sizeof(*mb);
				shmq_push(sendq, mb, NULL);
			}
			break;
		case CLOSE_BLOCK:
			handle_close(mb->fd);
			break;		
		default:
			break;
		}
	}
}

void run_worker_process(bind_config_t* bc, int bc_elem_idx, int n_inited_bc)
{
	bind_config_info_t* bc_elem = &(bc->configs[bc_elem_idx]);

	char prefix[10] = { 0 };
	int  len       = snprintf(prefix, 8, "%u", bc_elem->server_id);
	prefix[len] = '_';
	is_parent = 0;
	// 释放从父进程继承的资源
	close_shmq_pipe(bc, n_inited_bc, 1);
	shmq_destroy(bc_elem, n_inited_bc);
	net_exit();
	daemon_set_title("%s-%u", prog_name, bc_elem->server_id);	
	net_init(max_fd_num, 2000);
	do_add_conn(bc_elem->recvq.pipe_handles[0], fd_type_pipe, 0, 0);

	if ( handle_init(bc_elem) != 0 ) {
		ERROR_LOG("fail to init worker process. server_id=%u server_name=%s", bc_elem->server_id, bc_elem->server_name);
		goto fail;
	}
    int timeout = config_get_intval("net_loop_interval", 100);
    if (timeout < 0 || timeout > 1000)
        timeout = 100;
	while ( !stop || !handle_fini() ) {
		net_loop(timeout, page_size, 0);
	}

fail:
	do_destroy_shmq(bc_elem);
	net_exit();
	unregister_plugin();
	free_argv();
	free(prog_name);
	free(current_dir);
	exit(0);
}

void restart_child_process(bind_config_info_t* bc_elem)
{
	close(bc_elem->recvq.pipe_handles[1]);
	do_del_conn(bc_elem->sendq.pipe_handles[0], 2);
	do_destroy_shmq(bc_elem);

	shmq_create(bc_elem);

	bind_config_t* bc = get_bind_conf();
	int i = get_bind_conf_idx(bc_elem);
	pid_t pid;

	if ( (pid = fork ()) < 0 ) {
		BOOT_LOG("fork failed: %s", strerror(errno));
	} else if (pid > 0) { //parent process
		close_shmq_pipe(bc, i, 0);
		do_add_conn(bc_elem->sendq.pipe_handles[0], fd_type_pipe, 0, bc_elem);
        atomic_set(&child_pids[i], pid);
	} else { //child process
		run_worker_process(bc, i, bc->bind_num);
	}
}

