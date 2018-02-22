#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include "stdio.h"

#include "bindconf.h"
#include "config.h" 

bind_config_t bindconf; 

enum {
	bind_conf_max_field_num	= 4
};


int load_bind_file(const char* file_name)
{
	int		ret_code = -1;
	char*	buf;
    /*
	if ( mmap_config_file(file_name, &buf) > 0 ) {
		char* start = buf;
		char* end;
		char* field[bind_conf_max_field_num];
		struct bind_config_info* bc;

		size_t len = strlen(buf);
		while (buf + len > start) {
			end = strchr(start, '\n');
			if ( end && *end ) {
				*end = '\0';
			}
			if ( (*start != '#') && (str_split(0, start, field, bind_conf_max_field_num) == bind_conf_max_field_num) ) {
				bc = &(bindconf.configs[bindconf.bind_num]);
				// server
				bc->server_id = atoi(field[0]); // server id
				strncpy(bc->server_name, field[1], sizeof(bc->server_name) - 1); // server name
				strncpy(bc->bind_ip, field[2], sizeof(bc->bind_ip) - 1); // server ip
				bc->bind_port = atoi(field[3]); // server port
				// increase bind_num
				++(bindconf.bind_num);
			}
			start = end + 1;

            if (bindconf.bind_num > max_listen_fds) {
                BOOT_LOG("load bind file:%s", file_name);
			}
		}
		munmap(buf, len);
		ret_code = 0;
	}
    */
	return ret_code;
}

