extern "C"{
#include <MpNetServer/net_if.h>
}

extern "C" int	get_pkg_len(int fd, const void* pkg, int pkglen, int isparent)
{
    return pkglen;
}   


extern "C" int proc_pkg_from_client(void* data, int len, fdsession_t* fdsess)
{
    printf("recv:%s\n",(char*)data );
	return send_pkg_to_client(fdsess, data, len);
}

extern "C" void proc_pkg_from_serv(int fd, void* data, int len)
{
}

extern "C" void on_client_conn_closed(int fd)
{
}

 extern "C" void on_fd_closed(int fd)
{
}
