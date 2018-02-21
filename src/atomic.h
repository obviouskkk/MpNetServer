#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
  volatile int counter;
} atomic_t;


#define atomic_read(v)		((v)->counter)

#define atomic_set(v,i)		(((v)->counter) = (i))

#ifdef __cplusplus
} // end of extern "C"
#endif

#endif  // __ATOMIC_H__

