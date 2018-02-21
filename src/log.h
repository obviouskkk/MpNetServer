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

#define MAX_LOG_BUF 1024

static void write_log(int log_level, const char* fmt, ... )
{
    char* header[] = {"BOOT> ", "TRACE> ", "DEBUG> ", "ERROR> "};
    va_list ap;
    va_start(ap, fmt);
    char buf[MAX_LOG_BUF];
    strncpy (buf, header[log_level], strlen(header[log_level]));
    char body[MAX_LOG_BUF - 10];
    vsnprintf(body, MAX_LOG_BUF, fmt, ap);
    strcat(buf, body);
    strcat(buf, "\n");
    printf("%s", buf);
    va_end(ap);
}


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

enum LOG_LEVEL{
    BOOT  = 0,  
    TRACE = 1,
    DEBUG = 2,
    ERROR = 3,
};

void debug_log(const char* fmt, ...);
void error_log(const char* fmt, ...);
void trace_log(const char* fmt, ...);
void boot_log(const char* fmt, ...);

#ifdef __cplusplus 
    } 
#endif  // __cplusplus


#endif  // __LOG_H__

