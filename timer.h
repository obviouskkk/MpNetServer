/* ***********************************************************************

  > File Name: timer.h
  > author: obvious
  > Mail: zzy@taomee.com 
  > Created Time: Mon 01 May 2017 11:01:51 AM CST

 ********************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

struct timeval now;
struct tm      tm_cur;

static inline void renew_now()
{
	gettimeofday(&now, 0);
	localtime_r(&now.tv_sec, &tm_cur);
}
static inline const struct timeval* get_now_tv()
{
	return &now;
}

static inline const struct tm* get_now_tm()
{
    return &tm_cur;
}






