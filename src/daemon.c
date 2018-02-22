#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <linux/unistd.h>


#include "config.h"
#include "daemon.h"
#include "log.h"

atomic_t child_pids[max_listen_fds];

int max_fd_num;

volatile int	stop        = 0;
volatile int	restart     = 0;
volatile int    term_signal = 0;

char** saved_argv = NULL;
static char*	arg_start;
static char*	arg_end;
static char*	env_start;
static int		backgd_mode = 0;
static int		status;

static void sigterm_handler(int signo) 
{
	stop     = 1;
	restart  = 0;
    term_signal = 1;
}

static void sighup_handler(int signo) 
{
	restart  = 1;
	stop     = 1;
}

static void sigchld_handler(int signo, siginfo_t *si, void * p) 
{
    pid_t pid;
	while ((pid = waitpid (-1, &status, WNOHANG)) > 0) {
        int i;
        for (i = 0; i < 2; ++i) {
            if (atomic_read(&child_pids[i]) == pid) {
                atomic_set(&child_pids[i], 0);
				break;
            }
        }
	}
}


static inline void dup_argv(int argc, char** argv)
{
	saved_argv = malloc(sizeof(char*) * (argc + 1));
	if (!saved_argv) {
		return;
	}
	saved_argv[argc] = NULL;
	while (--argc >= 0) {
		saved_argv[argc] = strdup(argv[argc]);
	}
}
//设置资源限制 resource limit 
static void rlimit_reset( )
{	
	/*
	 struct rlimit {
	 rlim_t rlim_cur; // soft limit 
	 rlim_t rlim_max; // hard limit 
	 };
	  */
	struct rlimit rlim;

	max_fd_num = iniparser_getint(ini, "max_open_fd", 20000);

	/* raise open files */
	rlim.rlim_cur = max_fd_num;
	rlim.rlim_max = max_fd_num;
	if (setrlimit(RLIMIT_NOFILE, &rlim) == -1) {
		//ALERT_LOG("INIT FD RESOURCE FAILED");
	}

	/* allow core dump */
	rlim.rlim_cur = 1 << 30;
	rlim.rlim_max = 1 << 30;
	if (setrlimit(RLIMIT_CORE, &rlim) == -1) {
	//	ALERT_LOG("INIT CORE FILE RESOURCE FAILED");
	}
}


int daemon_start(int argc, char** argv )
{
	/*
	   struct sigaction {
	   union {
	   __sighandler_t sa_handler;
	   void (*_sa_sigaction)(int, struct siginfo *, void *);
	   } _u;
	   sigset_t sa_mask;
	   unsigned long sa_flags;
	   void (*sa_restorer)(void);
	   };
	 */
	struct sigaction sa;
	sigset_t sset;
	const char *style;

	rlimit_reset();

	memset(&sa, 0, sizeof(sa));
	/*SIGPIPE	在reader中止之后写Pipe的时候发送
	 SIG_ING 代表忽略SIGPIPE信号*/
	signal(SIGPIPE,SIG_IGN);	

	/*  SIGTERM 终止结束进程 15号信号 */
	sa.sa_handler = sigterm_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);

	/*  SIGHUP  终端退出 1号信号 */
	sa.sa_handler = sighup_handler;
	sigaction(SIGHUP, &sa, NULL);

	sa.sa_flags = SA_RESTART|SA_SIGINFO;//SIGCHLD 需要这么设置
	sa.sa_sigaction = sigchld_handler;
	sigaction(SIGCHLD, &sa, NULL);

	/* 用来将参数set信号集初始化并清空。*/
	sigemptyset(&sset);

	//sigaddset(&sset, SIGSEGV);
	sigaddset(&sset, SIGBUS);
	sigaddset(&sset, SIGABRT);
	sigaddset(&sset, SIGILL);
	sigaddset(&sset, SIGCHLD);
	sigaddset(&sset, SIGFPE);
	/* 这些信号不再被忽略*/
	sigprocmask(SIG_UNBLOCK, &sset, &sset);

	arg_start = argv[0];
	arg_end = argv[argc-1] + strlen (argv[argc - 1]) + 1;
	dup_argv(argc, argv);

	style = iniparser_getstring(ini, "run_mode", NULL);
	if (!style || !strcasecmp ("background", style)) {
		daemon (1, 1);
		backgd_mode = 1;
		BOOT_LOG (0, "switch to daemon mode");
        daemon_log(NULL);
	}
	return 0;

}


void clean_child_pids()
{
    int i;
    for (i = 0; i < max_listen_fds; ++i) {
        atomic_set(&child_pids[i], 0);
    }
}

void killall_children()
{
    int i;
    for (i = 0; i < bindconf.bind_num; ++i) {
        if (atomic_read(&child_pids[i]) != 0) {
            kill(atomic_read(&child_pids[i]), SIGTERM);
        }
    }

    /* wait for all child exit*/
WAIT_AGAIN:
    while (1) {
        int i;
        for (i = 0; i < bindconf.bind_num; ++i) {
            if (atomic_read(&child_pids[i]) != 0) {
                usleep(100);
                goto WAIT_AGAIN;
            }
        }
        return;
	}
}

void daemon_stop(void) 
{
	if (!backgd_mode) {
		printf("Server stopping...\n");
	}

	if (restart && prog_name && saved_argv) {
		DEBUG_LOG("%s", "Server restarting...");
		chdir(current_dir);
		execv(prog_name, saved_argv);
		DEBUG_LOG("%s", "Restart Failed...");
	}

	free_argv();
	free(prog_name);
	free(current_dir);
}

void daemon_set_title(const char* fmt, ...)
{
	char title[64];
	int i, tlen;
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(title, sizeof(title) - 1, fmt, ap);
	va_end(ap);

	tlen = strlen(title) + 1;
	if (arg_end - arg_start < tlen && env_start == arg_end) {
		char *env_end = env_start;
		for (i = 0; environ[i]; i++) {
			if(env_end == environ[i]) {
				env_end = environ[i] + strlen (environ[i]) + 1;
				environ[i] = strdup(environ[i]);
			} else
				break;
		}
		arg_end = env_end;
		env_start = NULL;
	}

	i = arg_end - arg_start;
	if (tlen == i) {
		strcpy (arg_start, title);
	} else if (tlen < i) {
		strcpy (arg_start, title);
		memset (arg_start + tlen, 0, i - tlen);
	} else {
		stpncpy(arg_start, title, i - 1)[0] = '\0';
	}
}
