/* ***********************************************************************

  > File Name: log.h
  > Author: zzy
  > Mail: 942744575@qq.com 
  > Created Time: Wed 21 Feb 2018 02:46:06 PM CST

 ********************************************************************** */

#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus 
extern "C" { 
#endif

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

#define MAX_LOG_BUF 1024

enum LOG_LEVEL{
    BOOT  = 0,  
    TRACE = 1,
    DEBUG = 2,
    ERROR = 3,
};

static char* get_date_char()
{
    time_t tv = time(NULL);
    struct tm * lt = localtime(&tv);
    static char buf[128] ;
    snprintf(buf, 128,  "%d-%d-%d", lt->tm_year + 1900, lt->tm_mon, lt->tm_mday);
    return buf;
}

static char* get_daytime_char()
{
    time_t tv = time(NULL);
    struct tm * lt = localtime(&tv);
    static char buf[128] ;
    snprintf(buf, 128,  "%d:%d:%d", lt->tm_hour, lt->tm_min, lt->tm_sec);
    return buf;

}

static void write_log(int log_level, const char* fmt, ... )
{
    char* header[] = {"BOOT> ", "TRACE> ", "DEBUG> ", "ERROR> "};
    va_list ap;
    va_start(ap, fmt);
    char buf[MAX_LOG_BUF];
    char body[MAX_LOG_BUF - 10];
    vsnprintf(body, MAX_LOG_BUF, fmt, ap);
    sprintf(buf, "%s[%s] %s\n", header[log_level], get_daytime_char(), body);
    //printf("%s", buf);
    write(1, buf, strlen(buf));
    va_end(ap);
}

int daemon_log(const char* log_dir);

#define DEBUG_LOG(fmt, args...) do \
{\
    write_log(DEBUG, fmt , ##args);\
}while (0)

#define BOOT_LOG(fmt, args...) do \
{\
    write_log(BOOT, fmt , ##args);\
}while (0)

#define ERROR_LOG(fmt, args...) do \
{\
    write_log(ERROR, fmt , ##args);\
}while (0)

#define TRACE_LOG(fmt, args...) do \
{\
    write_log(TRACE, fmt , ##args);\
}while (0)

#ifdef __cplusplus 
    } 
#endif  // __cplusplus

#endif  // __LOG_H__

