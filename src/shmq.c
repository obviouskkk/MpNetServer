#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <fcntl.h>

#include "bindconf.h"
#include "shmq.h"
#include "daemon.h"


inline struct shm_block * head_mb (const struct shm_queue *q)
{
	return (struct shm_block *) ((char *) q->addr + q->addr->head);
}


inline struct shm_block * tail_mb (const struct shm_queue *q)
{
	return (struct shm_block *) ((char *) q->addr + q->addr->tail);
}

int pipe_create(int pipe_handles[2])
{
	if (pipe (pipe_handles) == -1)
		return -1;

    int rflag, wflag;
	rflag = O_NONBLOCK | O_RDONLY;
	wflag = O_NONBLOCK | O_WRONLY;

	fcntl (pipe_handles[0], F_SETFL, rflag);
	fcntl (pipe_handles[1], F_SETFL, wflag);

	fcntl (pipe_handles[0], F_SETFD, FD_CLOEXEC);
	fcntl (pipe_handles[1], F_SETFD, FD_CLOEXEC);

	return 0;
}


static int do_shmq_create (struct shm_queue *q)
{
	
    q->addr = (shm_head_t *) mmap (NULL, q->length, PROT_READ | PROT_WRITE,MAP_SHARED | MAP_ANON, -1, 0);
	if (q->addr == MAP_FAILED)
		printf("mmap failed, %s", strerror (errno));

	q->addr->head = sizeof (shm_head_t);
	q->addr->tail = sizeof (shm_head_t);
	pipe_create (q->pipe_handles);
	return 0;
}


static int align_queue_tail (struct shm_queue *q)
{
	struct shm_block *pad;
	if (likely (q->addr->head >= q->addr->tail))
		return 0;

	pad = tail_mb (q);
	if (q->length - q->addr->tail < sizeof (shm_block_t)
			|| pad->type == PAD_BLOCK)
		q->addr->tail = sizeof (shm_head_t);

	return 0;
}

static int align_queue_head (struct shm_queue *q, const struct shm_block *mb)
{
	int tail_pos = q->addr->tail;
	int head_pos = q->addr->head;
	struct shm_block *pad;

	uint32_t surplus = q->length - head_pos;

	if (unlikely (surplus < mb->length))
	{
		//queue is full
		if (unlikely (tail_pos == sizeof (shm_head_t))) {
			ERROR_LOG("shm_queue is full,head=%d,tail=%d,mb_len=%d",
						head_pos, tail_pos, mb->length);
            exit (-1);
        }
		//bug
		else if (unlikely (q->addr->tail > head_pos))
		{
			// should be impossible
			ERROR_LOG("shm_queue bug, head=%d, tail=%d, mb_len=%d, total_len=%u",
						head_pos, tail_pos, mb->length, q->length);
			q->addr->tail = sizeof (shm_head_t);
			q->addr->head = sizeof (shm_head_t);
			//no pad mb
		}
		else if (unlikely (surplus < sizeof (shm_block_t)))
		{
			q->addr->head = sizeof (shm_head_t);
			//pad mb 
		}
		else
		{
			pad = head_mb (q);
			pad->type = PAD_BLOCK;
			pad->length = surplus;
			pad->id = 0;
			q->addr->head = sizeof (shm_head_t);
		}
	}
	return 0;
}

int shmq_pop(struct shm_queue* q, struct shm_block** mb)
{
	//queue is empty
	if (q->addr->tail == q->addr->head) {
		return -1;
	}

	align_queue_tail(q);

	//queue is empty
	if (q->addr->tail == q->addr->head) {
		return -1;
	}

	shm_block_t* cur_mb = tail_mb(q);
	int head_pos = q->addr->head;
	if (cur_mb->length > page_size)
		printf("too large packet, len=%d", cur_mb->length);
	*mb = cur_mb;
	q->addr->tail += cur_mb->length;
	TRACE_LOG("pop queue: q=%p length=%d tail=%d head=%d  fd=%d",
				q, cur_mb->length, q->addr->tail, q->addr->head, cur_mb->fd);
	return 0;
}

int shmq_push(shm_queue_t* q, shm_block_t* mb, const void* data)
{
	char* next_mb;

	assert(mb->length >= sizeof(shm_block_t));

	if (mb->length > page_size) {
		printf("too large packet, len=%d", mb->length);
		return -1;
	}

	if (align_queue_head(q, mb) == -1) {
		return -1;
	}

	int cnt;
	for (cnt = 0; cnt != 10; ++cnt) {
		if ( unlikely((q->addr->tail > q->addr->head)
						&& (q->addr->tail < q->addr->head + mb->length + page_size)) ) {
			DEBUG_LOG("queue [%p] is full, wait 5 microsecs: [cnt=%d]", q, cnt);
			usleep(5);
		} else {
			break;
		}
	}

	if (unlikely(cnt == 10)) {
		printf("queue [%p] is full.", q);
		return -1;
	}

	next_mb = (char*)head_mb(q);

	memcpy(next_mb, mb, sizeof (shm_block_t));
	if (likely(mb->length > sizeof (shm_block_t)))
		memcpy(next_mb + sizeof (shm_block_t), data, mb->length - sizeof (shm_block_t));

	q->addr->head += mb->length;
	write(q->pipe_handles[1], q, 1);

    TRACE_LOG("push queue: queue=%p,length=%d,tail=%d,head=%d,,fd=%d",q, mb->length, q->addr->tail, q->addr->head, mb->id, mb->fd); 
    return 0;
}


int shmq_create(struct bind_config_info* p)
{
	int err;

	p->sendq.length = iniparser_getint(ini, "shmq_length", 1 << 26);
	p->recvq.length = p->sendq.length;

	err = do_shmq_create(&(p->sendq)) | do_shmq_create(&(p->recvq));
	BOOT_LOG ("Create shared memory queue: %dMB", p->recvq.length / 1024 / 512);
	return err;
}


void close_shmq_pipe( struct  bind_config* bc, int idx, int is_child)
{
	if (is_child) {
		int i = 0;
		// close fds inherited from parent process
		for ( ; i != idx; ++i ) {
			close(bc->configs[i].recvq.pipe_handles[1]);
			close(bc->configs[i].sendq.pipe_handles[0]);
		}
	} else {
		close(bc->configs[idx].recvq.pipe_handles[0]);
		close(bc->configs[idx].sendq.pipe_handles[1]);
	}
}


void do_destroy_shmq(struct bind_config_info* bc_elem)
{
	struct shm_queue* q = &(bc_elem->sendq);
	// close send queue
	if ( q->addr ) {
		munmap(q->addr, q->length);
		q->addr = 0;
	}
	// close receive queue
	q = &(bc_elem->recvq);
	if ( q->addr ) {
		munmap(q->addr, q->length);
		q->addr = 0;
	}
}

void shmq_destroy(const bind_config_info_t* exclu_bc_elem, int max_shmq_num)
{
	bind_config_t* bc = get_bind_conf();

	int i = 0;
	for ( ; i != max_shmq_num; ++i ) {
		if (&(bc->configs[i]) != exclu_bc_elem) {
			do_destroy_shmq(&(bc->configs[i]));
		}
	}
}






