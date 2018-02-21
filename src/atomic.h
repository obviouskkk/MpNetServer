#ifndef __ATOMIC_H__
#define __ATOMIC_H__


typedef struct
{
  volatile int counter;
} atomic_t;


#define atomic_read(v)		((v)->counter)

#define atomic_set(v,i)		(((v)->counter) = (i))

#endif

