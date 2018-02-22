/* ***********************************************************************

  > File Name: config.c
  > Author: zzy
  > Mail: 942744575@qq.com 
  > Created Time: Thu 22 Feb 2018 11:04:14 AM CST

 ********************************************************************** */

#include "config.h" 
#include <assert.h>  
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>


dictionary *ini;
//创建内存映射，成功返回映射长度并把文件内容读到buf里，失败返回-1
int mmap_config_file(const char* file_name, char** buf)
{
    int ret_code = -1;

    int fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        return -1;
    }

    int len = lseek(fd, 0L, SEEK_END);//文件有多长
    lseek(fd, 0L, SEEK_SET);
    *buf = (char*)mmap(0, len + 1, 
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (*buf != MAP_FAILED) {
        read(fd, *buf, len);
        (*buf)[len] = 0;
        ret_code = len + 1;
    }
    close(fd);
 
    return ret_code;
}

//字符串切割 例如 ”run_style background \n“ 分取到别run_style background
//读取line这一行，取两个值，存在field中，返回实际取到的数目
int str_split(const char* ifs, char* line, char* field[], int n)
{
	static const char default_ifs[256]= { [' '] = 1, ['\t'] = 1, ['\r'] = 1, ['\n'] = 1, ['='] = 1 };

	if (ifs == 0) {
		ifs = default_ifs;
	}

	int i = 0;
	while (1) {
		while (ifs[(unsigned char)*line]) {
			++line;
		}
        //找到不是ifs的字符，如果直接找到\0退出循环
		if (*line == '\0') {
			break;
		}
		field[i++] = line;
		// 已经取到了足够的值,第二次
		if (i >= n) {
			line += strlen(line) - 1;
			while (ifs[(unsigned char)*line]) {//检查一下中间有没有ifs
				--line;
			}
			line[1] = '\0';//*(line+1) = '\0'
			break;
		}
		// 第一次 给尾部加一个 ‘\0’,
		while (*line && !ifs[(unsigned char)*line]) {
			++line;
		}
		if (!*line) {
			break;
		}
		*line++ = '\0';
	}
	return i;
}
