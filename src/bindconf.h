/* ***********************************************************************

  > File Name: bindconf.h
  > Author: zzy
  > Mail: 942744575@qq.com 
  > Created Time: Wed 21 Feb 2018 09:34:01 PM CST

 ********************************************************************** */

#ifndef __BINDCONF_H__
#define __BINDCONF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "shmq.h"
#include "config.h"
#include "net.h"
#include "log.h"

enum {
    max_listen_fds	= 500
};

typedef struct bind_config_info
{
    uint32_t server_id;
    char server_name[16];
    char bind_ip[16];
    uint32_t bind_port;
    uint8_t restart_cnt;
    shm_queue_t	sendq;
    shm_queue_t	recvq;
} bind_config_info_t;

typedef struct bind_config {
    int					bind_num;
    bind_config_info_t	configs[max_listen_fds];
} bind_config_t;

extern bind_config_t bindconf;

static inline bind_config_t* get_bind_conf()
{
    return &bindconf;
}

static inline int get_bind_conf_idx(const bind_config_info_t* bc_elem)
{
    return (bc_elem - &(bindconf.configs[0]));
}

int load_bind_file(const char* file_name);

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif  // __BINDCONF_H__

