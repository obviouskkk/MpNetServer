#ifndef __CONFIG_H__
#define __CONFIG_H__

int   mmap_config_file(const char* file_name, char** buf);

int config_init( const char* filename );


int config_get_intval(const char* key, int def);

char*  config_get_strval(const char* key);

int config_update_value(const char* key, const char* val);

int config_append_value(const char* key, const char* val); 

int str_split(const char* ifs, char* line, char* field[], int n);

void config_exit();

#endif
