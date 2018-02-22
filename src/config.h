/* ***********************************************************************

  > File Name: config.h
  > Author: zzy
  > Mail: 942744575@qq.com 
  > Created Time: Thu 22 Feb 2018 11:04:16 AM CST

 ********************************************************************** */

#ifndef __CONFIG_H__
#define __CONFIG_H__ 
#include <iniparser.h>

extern dictionary *ini;
int mmap_config_file(const char* file_name, char** buf);
int str_split(const char* ifs, char* line, char* field[], int n);

#endif  // __CONFIG_H__


