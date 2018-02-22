#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#include "daemon.h"
#include "config.h"
#include "dll.h"
#include "bindconf.h"
#include "timer.h"
#include "log.h"



char* prog_name;
char* current_dir;

static inline int show_usage()
{
	BOOT_LOG("Usage: %s conf", prog_name);
	exit(-1);
}

static inline void parse_args(int argc, char** argv)
{
	prog_name    = strdup(argv[0]);
	current_dir  = get_current_dir_name();
	if ( (argc < 2) || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-h") ) {
		show_usage();
	}
}

int main(int argc, char* argv[])
{
	parse_args(argc, argv);  //处理参数,显示usage等
	char *p_conf_file=argv[1];
    ini = iniparser_load(p_conf_file);
    /*   
	if (config_init(p_conf_file ) == -1) //解析bench.conf,存起来
    {
        BOOT_LOG("Failed to Parse File '%s'", argv[1]);
    }
    */
	daemon_start(argc, argv);

	renew_now();
	//解析bind.conf存导bind_config_t
	load_bind_file(iniparser_getstring(ini, "server:bind_conf", NULL));
	socket_timeout = iniparser_getint(ini, "server:cli_socket_timeout", 0);
	page_size      = iniparser_getint(ini, "server:incoming_packet_max_size", -1);
	send_buf_limit_size = iniparser_getint(ini, "server:send_buf_limit_size", 0);

	if (page_size <= 0) {
		page_size = def_page_size;
	}
	//dlopen,dlsym；注册句柄函数
	register_plugin(iniparser_getstring(ini, "dll_file", NULL));

	net_init(max_fd_num, max_fd_num);
	if(dll.init_service && dll.init_service(1) != 0)
	{
        BOOT_LOG("FAILED TO INIT PARENT PROCESS");
	}

	clean_child_pids();

	bind_config_t* bc = get_bind_conf();
		
	int i;
	pid_t pid;

	for ( i = 0; i != bc->bind_num; ++i ) {
		bind_config_info_t* bc_elem = &(bc->configs[i]);
		shmq_create(bc_elem);

		if ( (pid = fork ()) < 0 ) {
			BOOT_LOG("fork child process error");
		} else if (pid > 0) { //parent process
			close_shmq_pipe(bc, i, 0);
			do_add_conn(bc_elem->sendq.pipe_handles[0], fd_type_pipe, 0, bc_elem);
			net_start(bc_elem->bind_ip, bc_elem->bind_port, bc_elem);
            atomic_set(&child_pids[i], pid);
		} else { //child process
			listen_port = bc_elem->bind_port;
			strncpy(listen_ip, bc_elem->bind_ip, sizeof(listen_ip) - 1);
			run_worker_process(bc, i, i + 1);
		}
	}

    static int stop_count = 0;
	while (1) {
        if (unlikely(stop == 1 && term_signal == 1 )){
            DEBUG_LOG("SIG_TERM from pid=%d", getpid());
            if (dll.fini_service) 
                dll.fini_service(1);
            break;
        }
        if (net_loop(-1, page_size, 1) == -1)
            break;
	}

    killall_children();

	net_exit();
	unregister_plugin();
	shmq_destroy(0, bc->bind_num);
	daemon_stop();

	return 0;

}
