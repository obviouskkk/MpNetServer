/* ***********************************************************************

  > File Name: log.c
  > Author: zzy
  > Mail: 942744575@qq.com 
  > Created Time: Wed 21 Feb 2018 09:52:32 PM CST

 ********************************************************************** */

#include "log.h"

int log_init(const char* log_dir) 
{
    char* default_dir = "log";
    char* dir = log_dir == NULL ? default_dir: (char*)log_dir;
    int ret = mkdir(dir, 0755);
    if (ret == -1 && errno != EEXIST) {
        return -1;
    }
    const char* filename = get_date_char();
    char path[128];
    sprintf(path, "./%s/%s", dir, filename);
    int fd = open(path, O_RDWR|O_CREAT, 00644);
    lseek(fd, 0, SEEK_END);
    close(STDOUT_FILENO);
    dup(fd);
    return 0;
}





