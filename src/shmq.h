#ifndef __SHMQ_H__
#define __SHMQ_H__

#include <stdint.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>

#include "net.h"
#include "bindconf.h"

#define LOCKED_MASK 0x01
#define SLEEP_MASK  0x02
#define SHM_BLOCK_LEN_MAX   1<<23

#define unlikely(x)  __builtin_expect(!!(x), 0)
#define likely(x)  __builtin_expect(!!(x), 1)

enum {
	DATA_BLOCK = 0,
	PAD_BLOCK,
	FIN_BLOCK,	// child informs parent to close a connection
	OPEN_BLOCK,
	CLOSE_BLOCK	// parent informs child that a connection had been closed
};


typedef struct shm_head {
	volatile int head;
	volatile int tail;
} __attribute__ ((packed)) shm_head_t;


typedef struct shm_queue {
	shm_head_t* addr;
	uint32_t length;
	int pipe_handles[2];
} shm_queue_t;


typedef struct shm_block {
	uint32_t	id;
	uint32_t	fd;
	uint32_t	length;	// including data
	char		type;
	uint8_t		data[];
}__attribute__ ((packed))  shm_block_t;


struct bind_config_info;
struct bind_config;


int   shmq_create(struct bind_config_info* bc_elem);
void  do_destroy_shmq(struct bind_config_info* bc_elem);
void  shmq_destroy(const struct bind_config_info* exclu_bc_elem, int max_shmq_num);
int   shmq_push(shm_queue_t* q, shm_block_t* mb, const void* data);
int   shmq_pop(struct shm_queue *q, struct shm_block **mb);
char* shmblk_dump(const struct shm_block *mb);



extern void close_shmq_pipe(struct bind_config* bc, int idx, int is_child);

















#endif
